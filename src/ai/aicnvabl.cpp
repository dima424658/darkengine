/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvabl.cpp,v 1.7 2000/02/11 18:27:34 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <aicnvabl.h>

#include <lg.h>
#include <memall.h>
#include <mprintf.h>
#include <cfgdbg.h>

#include <appagg.h>

#include <aidebug.h>
#include <aicnvapi.h>
#include <aiactps.h>
#include <aiapiact.h>
#include <aiapibhv.h>
#include <aiapisnd.h>
#include <aiapisns.h>
#include <aigoal.h>
#include <aiprabil.h>
#include <aiprcore.h>
#include <aiutils.h>

#include <speech.h>
#include <schtype.h>

// Must be last header
#include <dbmem.h>

static const char* _g_converseStateNames[] =
{
   "Not Started",
   "Starting",
   "Requested Goal",
   "Started",
   "In Progress",
   "Waiting For Speech",
   "Finished Action",
   "Finished",
};

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitConverseAbility(IAIManager *)
{
   return TRUE;
}

///////////////////////////////////////

BOOL AITermConverseAbility()
{
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIConverse
//

cAIConverse::cAIConverse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIConverse::GetName()
{
   return "Converse";
}

///////////////////////////////////////

STDMETHODIMP cAIConverse::QueryInterface(REFIID id, void **ppI)
{
   if (id == IID_IAIConverse)
   {
      *ppI = (IAIConverse*)this;
      AddRef();
      return S_OK;
   }
   else
      return cAIAbility::QueryInterface(id, ppI);
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cAIConverse::AddRef(void)
{
   return cAIAbility::AddRef();
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cAIConverse::Release(void)
{
   return cAIAbility::Release();
}

///////////////////////////////////////
//
// Init()
//

STDMETHODIMP_(void) cAIConverse::Init()
{
   cAIAbility::Init();

   SetNotifications(kAICN_ActionProgress |
                    kAICN_Alertness);

   m_pConversation = NULL;
   m_pActionScript = NULL;
   m_nActions = 0;
   m_pCurrentAction = NULL;
   m_priority = kAIP_Low;
   m_state = kAIConvNotStarted;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::Term()
{
   SafeRelease(m_pConversation);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIConverse::SaveActions(ITagFile * pTagFile, cAIActions *pActions)
{
   // Overrided to not save the actions, since we don't want the pseudoscripts
   // saved until we save entire conversations.
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIConverse::LoadActions(ITagFile * pTagFile, cAIActions *pActions)
{
   // Overrided to not load the actions, since we don't want the pseudoscripts
   // saved until we save entire conversations.
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::OnSpeechStart(int hSchema)
{
   // record this as the latest speech sample that we started
   m_hSchema = hSchema;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::OnSpeechEnd(int hSchema)
{
   // if we were waiting for this speech sample to end...
   if (m_hSchema == hSchema)
   {
      m_hSchema = SCH_HANDLE_NULL;
      // if we are done with all other actions
      if (m_state == kAIConvWaitingForSpeech)
      {
         // Inform conversation that we are finished
         SetState(kAIConvFinishedAction);
         SafeRelease(m_pCurrentAction);
         m_pCurrentAction = NULL;
      }
   }
}

///////////////////////////////////////

DECLARE_TIMER(cAIConverse_OnActionProgress, Average);

STDMETHODIMP_(void) cAIConverse::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIConverse_OnActionProgress);

   // If we just completed our own goal...
   if (!(pAction->InProgress()) && (m_pCurrentAction == pAction))
      if (m_hSchema == SCH_HANDLE_NULL)
      {
         // Inform conversation that we are finished
         SetState(kAIConvFinishedAction);
         m_hSchema = SCH_HANDLE_NULL;
         SafeRelease(m_pCurrentAction);
         m_pCurrentAction = NULL;
      }
      else
         // we need to wait until we hear that the speech is ended
         SetState(kAIConvWaitingForSpeech);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::OnAlertness(ObjID source,
                                           eAIAwareLevel previous,
                                           eAIAwareLevel current,
                                           const sAIAlertness * pRaw)
{
   if (m_pConversation != NULL)
      m_pConversation->OnAlertness(current);
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIConverse::UpdateGoal(cAIGoal * pGoal)
{
   return kAIR_NoResult;
}

///////////////////////////////////////

STDMETHODIMP cAIConverse::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   if (m_state != kAIConvNotStarted)
   {
      cAIConverseGoal * pGoal = new cAIConverseGoal(this);
      pGoal->priority = m_priority;
      *ppNew = pGoal;
      if (m_state == kAIConvStarting)
         SetState(kAIConvRequestedGoal);
   }
   else
      *ppNew = NULL;
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAIAbility::OnGoalChange(pPrevious, pGoal);
   if (IsOwn(pGoal))
   {
      if (m_state == kAIConvRequestedGoal)
         SetState(kAIConvStarted);
   }
   else if (m_pConversation != NULL)
      SetState(kAIConvFinished);
}

///////////////////////////////////////

STDMETHODIMP cAIConverse::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   if (m_pActionScript != NULL)
   {
      ConfigSpew("ConvActor", ("Actor %d (obj %d) creating new pseudo script action\n", m_actorID, m_pAIState->GetID()));
      cAIPsdScrAction *pAction = CreatePsdScrAction();
      pAction->Set(m_pActionScript, m_nActions, 0);
      pNew->Append(pAction);
      if (m_pCurrentAction != NULL)
         m_pCurrentAction->Release();
      m_pCurrentAction = pAction;
      m_pCurrentAction->AddRef();
      for (int i=0; i<m_nActions; i++)
         if ((m_pActionScript[i].type == kAIPS_Play) && m_pActionScript[i].GetArg(0))
            SpeechInstallEndCallback(m_pAIState->GetID(), SpeechEndCallback);
      SetState(kAIConvInProgress);
   }
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::OnActionChange(IAIAction * pPrevious, IAIAction * pAction)
{
   cAIAbility::OnActionChange(pPrevious, pAction);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::Start(IAIConversation *pConversation, int actorID)
{
   ConfigSpew("ConvActor", ("Actor %d (obj %d) starting\n", actorID, m_pAIState->GetID()));
   m_pConversation = pConversation;
   m_pConversation->AddRef();
   m_pActionScript = NULL;
   m_nActions = 0;
   m_pCurrentAction = NULL;
   m_actorID = actorID;
   m_priority = kAIP_Low;
   SetState(kAIConvStarting);
   SpeechHalt(m_pAIState->GetID());
   SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::NewActions(sAIPsdScrAct *pActions, unsigned nActions)
{
   ConfigSpew("ConvActor", ("Actor %d (obj %d) %d new actions\n", m_actorID, m_pAIState->GetID(), nActions));
   m_hSchema = SCH_HANDLE_NULL;
   if (m_pActionScript != NULL)
      free(m_pActionScript);
   m_pActionScript = pActions;
   m_nActions = nActions;
   SignalAction();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::Terminate(void)
{
   ConfigSpew("ConvActor", ("Actor %d (obj %d) terminating\n", m_actorID, m_pAIState->GetID()));
   SignalGoal();
   SafeRelease(m_pConversation);
   m_pConversation = NULL;
   SetState(kAIConvNotStarted);
   SpeechUninstallEndCallback(m_pAIState->GetID(), SpeechEndCallback);
}

///////////////////////////////////////

STDMETHODIMP_(int) cAIConverse::GetCurrentAction(void)
{
   if (m_pCurrentAction == NULL)
      return -1;
   Assert_(m_pActionScript != NULL);
   return ((cAIPsdScrAction*)m_pCurrentAction)->GetCurrentStep();
}

///////////////////////////////////////

void cAIConverse::SetState(eAIConverseState state)
{
   eAIConverseState oldState = state;

   ConfigSpew("ConvActor", ("Actor %d (obj %d) to state %s\n", m_actorID, m_pAIState->GetID(), _g_converseStateNames[state]));
   // if we're finished, then free the action structure that was malloc-ed by the conversation
   // (it's a little easier for us to do that here)
   if (state == kAIConvFinishedAction)
   {
      free(m_pActionScript);
      m_pActionScript = NULL;
   }
   m_state = state;
   if (m_pConversation != NULL)
      m_pConversation->OnStateChange(m_actorID, oldState, state);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConverse::SetPriority(eAIPriority priority)
{
   m_priority = priority;
}

///////////////////////////////////////

void cAIConverse::SpeechEndCallback(ObjID speakerID, int hSchema, ObjID schemaID)
{
   AutoAppIPtr(AIManager);
   cAIConverse* pConverse;
   IInternalAI* pInternalAI;

   pInternalAI = (IInternalAI*)(pAIManager->GetAI(speakerID));
   if (!pInternalAI)
   {
      AIWatch1(Sound, speakerID, "Received speech end callback for non-existent AI (%d)", schemaID);
      return;
   }
   Assert_(pInternalAI);
   if (SUCCEEDED(pInternalAI->GetComponent("Converse", (IAIComponent**)&pConverse)))
   {
      pConverse->OnSpeechEnd(hSchema);
      pConverse->Release();
   }
   pInternalAI->Release();
}

///////////////////////////////////////////////////////////////////////////////
