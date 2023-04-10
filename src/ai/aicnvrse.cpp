/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvrse.cpp,v 1.5 1998/10/10 14:01:42 TOML Exp $
//
//

#include <lg.h>
#include <appagg.h>
#include <cfgdbg.h>

#include <aiapi.h>
#include <aiapiiai.h>
#include <aiapicmp.h>
#include <aicnvabl.h>
#include <aicnvrse.h>
#include <aiprconv.h>

#include <dlisttem.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

cAIConversor::cAIConversor(int actorID, ObjID objID):
   m_actorID(actorID),
   m_objID(objID)
{
   ConfigSpew("ConvFlow", ("Creating conversor for obj %d (actor %d)\n", objID, actorID));
   AutoAppIPtr(AIManager);
   IInternalAI *pInternalAI = (IInternalAI*)(pAIManager->GetAI(objID));
   tAIIter iter;
   IAIComponent *pComponent = pInternalAI->GetFirstComponent(&iter);
   BOOL found = FALSE;

   while ((pComponent != NULL) && !found)
   {
      found = SUCCEEDED(pComponent->QueryInterface(IID_IAIConverse, (void**)&m_pAbility));
      pComponent->Release();
      if (!found)
         pComponent = pInternalAI->GetNextComponent(&iter);
   }
   if (!found)
      Warning(("Actor %d does not support the converse ability\n", objID));
   pInternalAI->GetComponentDone(&iter);
   SafeRelease(pInternalAI);
}

///////////////////////////////////////////////////////////////////////////////

cAIConversor::~cAIConversor()
{
   ConfigSpew("ConvFlow", ("Destroying conversor for obj %d (actor %d)\n", m_objID, m_actorID));
   SafeRelease(m_pAbility);
}

///////////////////////////////////////////////////////////////////////////////

cAIConversation::cAIConversation(ObjID conversationID):
   m_objID(conversationID),
   m_step(-1),
   m_flags(0)
{
   ObjID objID;
   cAIConversor *pConversor;
   int actorIDs[kAICA_Num];
   ConfigSpew("ConvFlow", ("Creating new conversation from obj %d\n", conversationID));

   if (!g_pAIConversationProperty->Get(m_objID, &m_pConversationDesc))
   {
      Warning(("Object %d is not a conversation\n", conversationID));
      m_flags |= kAIConversationKillMe;
      return;
   }

   // Figure out number of steps in the conversation
   for (m_numSteps=0; (m_numSteps<kAIMaxConvSteps-1) && (m_pConversationDesc->GetActor(m_numSteps, 0) != kAICA_None); m_numSteps++);

   AutoAppIPtr(AIManager);
   cAutoIPtr<IAIConversationManager> pConversationManager = pAIManager->GetConversationManager();
   
   // Find all actor objects and add to conversor list
   int nActors = pConversationManager->GetActorIDs(conversationID, actorIDs);
   for (int i=0; i<nActors; i++)
   {
      if (!FindConversor(actorIDs[i], &pConversor))
         if (pConversationManager->GetActorObj(conversationID, actorIDs[i], &objID))
            NewConversor(actorIDs[i], objID);
         else
         {
            Warning(("Attempt to start conversation with missing actor %d\n", actorIDs[i]));
            m_flags |= kAIConversationKillMe;
         }
   }

   // start all conversors
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();
   while (pConversorNode != NULL)
   {
      if (!pConversorNode->item->m_pAbility)
      {
         m_flags |= kAIConversationKillMe;
         break;
      }
      pConversorNode->item->m_pAbility->Start(this, pConversorNode->item->m_actorID);
      pConversorNode = pConversorNode->GetNext();
   }
}

///////////////////////////////////////////////////////////////////////////////

cAIConversation::~cAIConversation()
{
   ConfigSpew("ConvFlow", ("Destroying conversation (conv obj %d)\n", m_objID));
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAIConversation::FindConversor(int actorID, cAIConversor** ppConversor)
{
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();

   while (pConversorNode != NULL)
   {
      if (pConversorNode->item->m_actorID == actorID)
      {
         *ppConversor = pConversorNode->item;
         return TRUE;
      }
      pConversorNode = pConversorNode->GetNext();
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

void cAIConversation::NewConversor(int actorID, ObjID objID)
{
   cAIConversorNode *pNewNode = new cAIConversorNode();
   pNewNode->item = new cAIConversor(actorID, objID);
   m_conversors.Append(pNewNode);
}

///////////////////////////////////////////////////////////////////////////////

void cAIConversation::DestroyAllConversors(void)
{
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();

   while (pConversorNode != NULL)
   {
      delete pConversorNode->item;
      pConversorNode->item = NULL;
      pConversorNode = pConversorNode->GetNext();
   }
   m_conversors.DestroyAll();
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAIConversation::ReadyToStart(void)
{
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();

   while (pConversorNode != NULL)
   {
      if (pConversorNode->item->m_state != kAIConvStarted)
         return FALSE;
      pConversorNode = pConversorNode->GetNext();
   }
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Are we ready for the next step?
// Currently just checks that all actors are done with their actions
// 
BOOL cAIConversation::FinishedWithStep(void)
{
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();
   int currentAction;
   int action;

   while (pConversorNode != NULL)
   {
      // check finished or not given anything to do yet
      if ((pConversorNode->item->m_state != kAIConvFinishedAction) && (pConversorNode->item->m_state != kAIConvStarted))
      {
         // if not finished, check to see if there are only non-blocking actions remaining
         action = 0;
         currentAction = pConversorNode->item->m_pAbility->GetCurrentAction();
         for (int i=0; i<kAIMaxConvActions; i++)
         {
            if (m_pConversationDesc->steps[m_step][i].actor == pConversorNode->item->m_actorID)
            {
               if (action >= currentAction)
                  if (!((m_pConversationDesc->steps[m_step][i].flags)&kAICAct_NoBlock))
                     return FALSE;
               ++action;
            }
         }
      }
      pConversorNode = pConversorNode->GetNext();
   }
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Are we ready for the next step?
// Currently just checks that all actors are done with their actions
// 
void cAIConversation::SetPriority(eAIPriority priority)
{
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();

   while (pConversorNode != NULL)
   {
      // check finished or not given anything to do yet
      pConversorNode->item->m_pAbility->SetPriority(priority);
      pConversorNode = pConversorNode->GetNext();
   }
}

///////////////////////////////////////////////////////////////////////////////
// Start all the actions for the given conversation step
// returns number of actions started
//
int cAIConversation::StartActions(int step)
{
   sAIConvAction* pConvAction;
   cAIConversor* pConversor;
   sAIPsdScrAct *pActions;
   int nActions;
   int count = 0;
   int nCopied = 0;

   // start new actions
   // for each conversor...
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();
   while (pConversorNode != NULL)
   {
      pConversor = pConversorNode->item;
      nActions = 0;
      // find how many actions there are for you in this step
      for (int i=0; i<kAIMaxConvActions; i++)
      {
         pConvAction = m_pConversationDesc->GetAction(step, i);
         if (pConvAction->actor == pConversor->m_actorID)
            ++nActions;
      }
      if (nActions>0)
      {
         // create the action array for the conversor
         // this gets freed by the ability when it is done
         pActions = (sAIPsdScrAct*)malloc(nActions*sizeof(sAIPsdScrAct));
         nCopied = 0;
         for (i=0; i<kAIMaxConvActions; i++)
         {
            pConvAction = m_pConversationDesc->GetAction(step, i);
            if (pConvAction->actor == pConversor->m_actorID)
            {
               Assert_(nCopied<nActions);
               pActions[nCopied++] = pConvAction->act;
            }
         }
         Assert_(nCopied == nActions);
         // hand off action array to conversor
         pConversor->m_pAbility->NewActions(pActions, nActions);
         count += nActions;
      }
      pConversorNode = pConversorNode->GetNext();
   }
   return count;
}

///////////////////////////////////////////////////////////////////////////////
// Start up the next step in the conversation
//

void cAIConversation::StartNextStep(void)
{

   ConfigSpew("ConvFlow", ("Conversation %d: starting step %d\n", m_objID, m_step+1));

   // increment steps
   ++m_step;
   Assert_(m_step<=kAIMaxConvSteps-1);
   if (m_step == m_numSteps)
      return;
   StartActions(m_step);
}

///////////////////////////////////////////////////////////////////////////////

void cAIConversation::End(void)
{
   ConfigSpew("ConvFlow", ("Ending conversation %d\n", m_objID));

   m_flags |= kAIConversationDying;
   cAIConversorNode* pConversorNode = m_conversors.GetFirst();
   while (pConversorNode != NULL)
   {
      pConversorNode->item->m_pAbility->Terminate();
      pConversorNode = pConversorNode->GetNext();
   }
   DestroyAllConversors();
   AutoAppIPtr(AIManager);
   cAutoIPtr<IAIConversationManager> pConversationManager = pAIManager->GetConversationManager();
   pConversationManager->NotifyConversationEnd(m_objID);
   return;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cAIConversation::Frame(void)
{
   if (m_flags&kAIConversationKillMe)
      End();
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cAIConversation::OnAlertness(eAIAwareLevel awareness)
{
   if (awareness>m_pConversationDesc->abortLevel)
   {
      if (!(m_flags&kAIConversationAlertBreakOut))
      {
         m_step = kAIMaxConvSteps-1;
         if (StartActions(m_step)>0)
         {
            SetPriority(m_pConversationDesc->abortPriority);
            ConfigSpew("ConvFlow", ("Conversation %d: alert causes termination sequence\n", m_objID));
            m_flags |= kAIConversationAlertBreakOut;
         }
      }
      if (!(m_flags&kAIConversationAlertBreakOut))
      {
         ConfigSpew("ConvFlow", ("Conversation %d: alert causes conv end\n", m_objID));
         End();
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cAIConversation::OnStateChange(int actorID, eAIConverseState newState, eAIConverseState oldState)
{
   cAIConversor *pConversor;

   if (!FindConversor(actorID, &pConversor))
   {
      Warning(("No such actor %d\n", actorID));
      return;
   }
   pConversor->m_state = newState;
   switch (newState)
   {
   case kAIConvStarted:
      if (m_step == -1)
      {
         // start the first step
         if (ReadyToStart())
            StartNextStep();
      }
      break;
   // we've finished an individual action
   case kAIConvFinishedAction:
      if (FinishedWithStep())
         if (m_step<m_numSteps-1)
            StartNextStep();
         else
            End();  
      break;
   // if any actor finishes (loses goal, etc), then kill the whole conversation
   case kAIConvFinished:
      End();  
      break;
   }
}
