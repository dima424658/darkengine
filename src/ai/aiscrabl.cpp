/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiscrabl.cpp,v 1.20 2000/02/11 18:28:00 bfarquha Exp $
//
// @TBD (toml 10-05-98): save/load flags when goal save/load in
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <appagg.h>

#include <bodscrpt.h>
#include <frobctrl.h>
#include <scrptapi.h>
#include <linkbase.h>
#include <relation.h>

#include <contain.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aidebug.h>
#include <aigoal.h>
#include <aiscrabl.h>
#include <aiscrm.h>
#include <aisignal.h>
#include <aitagtyp.h>

// Must be last header
#include <dbmem.h>

#define kAIScriptGoalExpiration 1500

//////////////////////////////////////////////////////////////////////////////

enum eAICustomScriptGoals
{
   kAICSG_Transition,
   kAICSG_Motion
};

//////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitScriptAbility(IAIManager *)
{
   return TRUE;
}

///////////////////////////////////////

BOOL AITermScriptAbility()
{
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBasicScript
//

cDynArray<cAIBasicScript *> cAIBasicScript::gm_ScriptAbilities;

///////////////////////////////////////

cAIBasicScript::cAIBasicScript()
 : m_pPendingGoal(NULL),
   m_pPendingData(new cMultiParm),
   m_pCurrentData(new cMultiParm)
{
   gm_ScriptAbilities.Append(this);
}

///////////////////////////////////////

cAIBasicScript::~cAIBasicScript()
{
   for (int i = 0; i < gm_ScriptAbilities.Size(); i++)
   {
      if (gm_ScriptAbilities[i] == this)
      {
         gm_ScriptAbilities.FastDeleteItem(i);
         break;
      }
   }
   SafeRelease(m_pPendingGoal);
   delete m_pPendingData;
   delete m_pCurrentData;
}

///////////////////////////////////////

cAIBasicScript * cAIBasicScript::AccessBasicScript(ObjID id)
{
   for (int i = 0; i < gm_ScriptAbilities.Size(); i++)
   {
      if (gm_ScriptAbilities[i]->GetID() == id)
      {
         return gm_ScriptAbilities[i];
      }
   }
   return NULL;
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIBasicScript::GetName()
{
   return "Scripting ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBasicScript::Init()
{
   cAIAbility::Init();

   SetNotifications(kAICN_ActionProgress |
                    kAICN_Alertness |
                    kAICN_HighAlert |
                    kAICN_GoalChange |
                    kAICN_ActionChange |
                    kAICN_Signal |
                    kAICN_ModeChange);
}

///////////////////////////////////////
//
// Save/load
//
//

STDMETHODIMP_(BOOL) cAIBasicScript::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_BasicScript, 0, 0, pTagFile))
   {
      BaseAbilitySave(pTagFile);

      BOOL fHavePending = !!m_pPendingGoal;

      AITagMove(pTagFile, &fHavePending);
      if (fHavePending)
      {
         eAIGoalType type = m_pPendingGoal->GetType();
         AITagMove(pTagFile, &type);
         m_pPendingGoal->Save(pTagFile);
      }

      AITagMoveMultiParm(pTagFile, m_pPendingData);
      AITagMoveMultiParm(pTagFile, m_pCurrentData);
      AITagMoveString(pTagFile, &m_Motion);
      m_TransitionTimer.Save(pTagFile);

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIBasicScript::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_BasicScript, 0, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      BOOL fHavePending;

      AITagMove(pTagFile, &fHavePending);
      if (fHavePending)
      {
         eAIGoalType type;
         AITagMove(pTagFile, &type);

         m_pPendingGoal = AICreateGoalFromType(type, this);

         if (!m_pPendingGoal)
            return FALSE;

         m_pPendingGoal->Load(pTagFile);
      }

      AITagMoveMultiParm(pTagFile, m_pPendingData);
      AITagMoveMultiParm(pTagFile, m_pCurrentData);
      AITagMoveString(pTagFile, &m_Motion);
      m_TransitionTimer.Save(pTagFile);

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIBasicScript::SaveGoal(ITagFile * pTagFile, cAIGoal * pGoal)
{
   if (!cAINonCombatAbility::SaveGoal(pTagFile, pGoal))
      return FALSE;
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIBasicScript::LoadGoal(ITagFile * pTagFile, cAIGoal ** ppGoal)
{
   if (!cAINonCombatAbility::LoadGoal(pTagFile, ppGoal))
      return FALSE;
   return TRUE;
}

///////////////////////////////////////

#define kTransitionTime 250

STDMETHODIMP_(void) cAIBasicScript::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAINonCombatAbility::OnGoalChange(pPrevious, pGoal);
   if (IsOwn(pGoal))
   {
      if (pGoal->GetType() == kAIGT_Idle && pGoal->ownerData == kAICSG_Transition)
         m_TransitionTimer.Set(AICustomTime(kTransitionTime));
      else
         *m_pCurrentData = *m_pPendingData;

      if (pGoal->GetType() != kAIGT_Idle || pGoal->ownerData != kAICSG_Motion)
         m_Motion.Empty();
   }
   else if (IsLosingControl(pPrevious, pGoal))
   {
      m_flags &= ~(kTransitionGoal | kLocoFailed);
      if (!m_Motion.IsEmpty())
      {
         AutoAppIPtr(ScriptMan);
         sBodyMsg bodyMessage(GetID(), sBodyMsg::kMotionEnd, m_Motion, NULL);
         pScriptMan->SendMessage(&bodyMessage);
         m_Motion.Empty();
      }
   }
}

////////////////////////////////////////

STDMETHODIMP_(void) cAIBasicScript::OnModeChange(eAIMode prev, eAIMode mode)
{
   AutoAppIPtr(ScriptMan);
   sAIModeChangeMsg* msg = new sAIModeChangeMsg(GetID(),mode,prev);
   pScriptMan->PostMessage(msg);
   msg->Release();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBasicScript::OnActionChange(IAIAction * pPrevious, IAIAction * pAction)
{
   cAIAbility::OnActionChange(pPrevious, pAction);
}

///////////////////////////////////////

DECLARE_TIMER(cAIBasicScript_OnActionProgress, Average);

STDMETHODIMP_(void) cAIBasicScript::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIBasicScript_OnActionProgress);

   if (IsOwn(pAction))
   {
      if (!pAction->InProgress())
      {
         if (pAction->GetType() == kAIAT_Motion && !m_Motion.IsEmpty())
         {
            AIWatch2(Flow,
                     GetID(),
                     "Script result -- PlayMotion(%s) %s",
                     m_Motion.operator const char *(), (pAction->GetResult() == kAIR_Success) ? "success" : "failure");
            AutoAppIPtr(ScriptMan);
            sBodyMsg bodyMessage(GetID(), sBodyMsg::kMotionEnd, ((cAIMotionAction *)pAction)->GetName(), NULL);
            pScriptMan->SendMessage(&bodyMessage);
            m_Motion.Empty();
         }
         else if (pAction->GetType() == kAIAT_Locomote && pAction->GetResult() != kAIR_Success)
         {
            m_flags |= kLocoFailed;
         }
      }

   }
   else if (!pAction->InProgress())
   {
      if (m_pPendingGoal)
         SignalGoal();
   }

   cAINonCombatAbility::OnActionProgress(pAction);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBasicScript::OnAlertness(ObjID source, eAIAwareLevel previous,  eAIAwareLevel current, const sAIAlertness * pRaw)
{
   cAINonCombatAbility::OnAlertness(source, previous,  current, pRaw);

   AutoAppIPtr(ScriptMan);
   sAIAlertnessMsg * pMsg = new sAIAlertnessMsg(GetID(), (eAIScriptAlertLevel)current, (eAIScriptAlertLevel)previous);
   pScriptMan->PostMessage(pMsg);
   pMsg->Release();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBasicScript::OnHighAlert(ObjID source, eAIAwareLevel previous,  eAIAwareLevel current, const sAIAlertness * pRaw)
{
   AutoAppIPtr(ScriptMan);
   sAIHighAlertMsg * pMsg = new sAIHighAlertMsg(GetID(), (eAIScriptAlertLevel)current, (eAIScriptAlertLevel)previous);
   pScriptMan->PostMessage(pMsg);
   pMsg->Release();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBasicScript::OnSignal(const sAISignal * pSignal)
{
   AutoAppIPtr(ScriptMan);
   sAISignalMsg msg(GetID(), pSignal->name);
   pScriptMan->SendMessage(&msg);
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIBasicScript::UpdateGoal(cAIGoal * pGoal)
{
   cAINonCombatAbility::UpdateGoal(pGoal);

   switch (pGoal->GetType())
   {
      case kAIGT_Goto:
      {
         // @TBD (toml 05-25-98): not right but ok for now
         if (!pGoal->InProgress() || (m_flags & kLocoFailed))
         {
            AIWatch2(Flow,
                     GetID(),
                     "Script result -- GotoObjLoc(%d) %s",
                     pGoal->object, (pGoal->result == kAIR_Success && !(m_flags & kLocoFailed)) ? "success" : "failure");

            sAIObjActResultMsg * pMsg = new sAIObjActResultMsg(GetID(), kAIGoto, pGoal->object, kActionDone, *m_pCurrentData);

            m_flags &= ~kLocoFailed;

            AutoAppIPtr(ScriptMan);
            pScriptMan->PostMessage(pMsg);
            pMsg->Release();
            m_flags |= kTransitionGoal;
            SignalGoal();
         }
         break;
      }

      case kAIGT_Idle:
      {
         switch (pGoal->ownerData)
         {
            case kAICSG_Transition:
            {
               if (m_TransitionTimer.Expired())
               {
                  pGoal->result = kAIR_Success;
                  SignalGoal();
               }

               break;
            }

            case kAICSG_Motion:
            {
               if (m_Motion.IsEmpty())
               {
                  pGoal->result = kAIR_Success;
                  // If the motion completed, m_motion will be empty
                  m_flags |= kTransitionGoal;
                  SignalGoal();
               }
               break;
            }
         }
         break;
      }

      default:
         CriticalMsg("Unknown script ability goal");
   }

   return pGoal->result;
}

///////////////////////////////////////

STDMETHODIMP cAIBasicScript::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
#ifndef SHIP
   if ((m_flags & kTransitionGoal) && m_pPendingGoal)
      Warning(("Cannot have transition goal and pending goal at the same time\n"));
#endif

   *ppGoal = NULL;

   if (m_pPendingGoal)
   {
      if (m_pPendingGoal->priority == kAIP_None) // workaround for motions, where priority is not provided @TBD: motions should carry priority
      {
         if (IsOwn(pPrevious))
            m_pPendingGoal->priority = pPrevious->priority;
         else
            m_pPendingGoal->priority = kAIP_Normal;
      }
      m_pPendingGoal->expiration = AIGetTime() + kAIScriptGoalExpiration;
      *ppGoal = m_pPendingGoal;
      m_pPendingGoal = NULL;
   }
   else if (pPrevious)
   {
      if (m_flags & kTransitionGoal)
      {
         *ppGoal = new cAIIdleGoal(this);
         (*ppGoal)->priority = pPrevious->priority;
         (*ppGoal)->ownerData = kAICSG_Transition;
         m_flags &= ~kTransitionGoal;
      }
      else
      {
         if (pPrevious->InProgress() && AIGetTime() < pPrevious->expiration)
         {
            pPrevious->AddRef();
            *ppGoal = pPrevious;
         }
      }
   }

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIBasicScript::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   // If we're in a transition state, do nothing
   if (pGoal->GetType() == kAIGT_Idle && pGoal->ownerData == kAICSG_Transition)
      return S_OK;

   m_flags &= ~kLocoFailed;

// @TBD (toml 05-25-98): not right but ok for now
   if (!m_Motion.IsEmpty())
   {
      cAIMotionAction * pAction = CreateMotionAction();
      pAction->SetByName(m_Motion);
      pNew->Append(pAction);
   }
   else
   {
// @TBD (toml 05-25-98): should be getting from b-set
      cAILocoAction * pAction = CreateLocoAction();
      pAction->SetFromGoal((cAIGotoGoal *)pGoal);
      pNew->Append(pAction);
   }

   //mprintf("Suggest action");
   return S_OK;
}

///////////////////////////////////////

BOOL cAIBasicScript::GotoObjLoc(ObjID              target,
                                eAISpeed           speed,
                                eAIPriority        priority,
                                const cMultiParm & dataToSendOnReach)
{
   SafeRelease(m_pPendingGoal);

   cAIGotoGoal * pGoal = new cAIGotoGoal(this);

   pGoal->priority = priority;
   pGoal->object   = target;
   pGoal->speed    = speed;
   pGoal->LocFromObj();

   m_pPendingGoal  = pGoal;
   *m_pPendingData = dataToSendOnReach;
   m_flags &= ~kTransitionGoal;
   SignalGoal();

   AIWatch3(Flow,
            GetID(),
            "Script -- GotoObjLoc(%d, %s, %s)",
            target, AIGetSpeedName(speed), AIGetPriorityName(priority));

   return TRUE;
}

///////////////////////////////////////

BOOL cAIBasicScript::FrobObj(ObjID              objIdTarget,
                             ObjID              objIdWith,
                             eAIPriority        priority,
                             const cMultiParm & dataToSendOnReach)
{
   AIWatch3(Flow,
            GetID(),
            "Script -- FrobObj(%d, %d, %s) (immediate success)",
            objIdTarget, objIdWith, AIGetPriorityName(priority));

// @TBD (toml 05-25-98): not right but ok for e3

// @TBD (toml 01-02-98): no no no no no no no no no
// @TBD (dc 01-12-98): good call, anyway, now we might want the AI data as
//   frobber, ie. to know who is doing the frob, in case it is supposed to
//   damage or something.  ie. maybe we need a frob owner as well...
   sFrobActivate frob_info;
   frob_info.frobber=GetID();
   frob_info.ms_down=1; // since we dont have time data...

   if (objIdWith == OBJ_NULL)
   {
      // in this case, we want to see if we are inv
      AutoAppIPtr_(ContainSys,pContainSys);
      if (pContainSys->Contains(GetID(),objIdTarget))
         frob_info.src_loc=kFrobLocInv;
      else
         frob_info.src_loc=kFrobLocWorld;
      frob_info.src_obj=objIdTarget;
      frob_info.dst_loc=kFrobLocNone;
      frob_info.dst_obj=OBJ_NULL;
   }
   else
   {
      frob_info.src_loc=kFrobLocTool;
      frob_info.src_obj=objIdWith;
      frob_info.dst_loc=kFrobLocWorld;
      frob_info.dst_obj=objIdTarget;
   }

   FrobInstantExecute(&frob_info);

   sAIObjActResultMsg * pMsg = new sAIObjActResultMsg(GetID(), kAIFrob, objIdTarget, kActionDone, dataToSendOnReach);

   AutoAppIPtr(ScriptMan);
   pScriptMan->PostMessage(pMsg);
   pMsg->Release();

   return FALSE;
}

///////////////////////////////////////

BOOL cAIBasicScript::PlayMotion(const char *pszName)
{
   AIWatch1(Flow,
            GetID(),
            "Script -- PlayMotion(%s)",
            pszName);

   SafeRelease(m_pPendingGoal);

   m_pPendingGoal = new cAIIdleGoal(this);
   m_pPendingGoal->ownerData = kAICSG_Motion;
   m_pPendingGoal->priority = kAIP_None;
   *m_pPendingData = "";
   m_Motion = pszName;
   m_flags &= ~kTransitionGoal;
   SignalGoal();
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
