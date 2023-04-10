/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aideath.cpp,v 1.16 2000/02/11 18:27:36 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <dmgbase.h>
#include <dmgmodel.h>
#include <mtagvals.h>

#include <aiactmot.h>
#include <aiapibhv.h>
#include <aiapisnd.h>
#include <aideath.h>
#include <aigoal.h>
#include <aisndtyp.h>
#include <aiprutil.h>
#include <aidebug.h>
#include <aitagtyp.h>

#include <chevkind.h>
#include <stimtype.h>
#include <stimbase.h>
#include <physapi.h>
#include <ghostapi.h>

#include <creatext.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDeath
//

STDMETHODIMP_(const char *) cAIDeath::GetName()
{
   return "Death";
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIDeath::Init()
{
   // Intentially not calling base -- want no goals or notifications by default
   SetNotifications(kAICN_Death);

   m_DeathCause = kAIDC_Invalid;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDeath::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Death, 0, 0, pTagFile))
   {
      BaseAbilitySave(pTagFile);

      AITagMove(pTagFile, &m_DeathCause);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDeath::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Death, 0, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);

      AITagMove(pTagFile, &m_DeathCause);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

DECLARE_TIMER(cAIDeath_OnActionProgress, Average);

STDMETHODIMP_(void) cAIDeath::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIDeath_OnActionProgress);

   if (IsOwn(pAction) && !pAction->InProgress())
   {
      CreatureBeDead(m_pAI->GetObjID());
      m_pAI->Kill();
   }
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIDeath::OnDeath(const sDamageMsg * pMsg)
{
   ObjID deathStim = pMsg->data.slay;

   if (deathStim == OBJ_NULL)
   {
      sStimEvent *stim = (sStimEvent *)pMsg->Find(kEventKindStim);
      if (stim)
         deathStim = stim->kind;
   }

   if (AIGetStimKnockout(deathStim))
      m_DeathCause = kAIDC_Knockout;
   else
      m_DeathCause = kAIDC_Damage;

   AIWatch1(Death, m_pAI->GetObjID(), "slain due to %d stim", deathStim);

   SignalGoal();
}

//////////////////////////////////////

STDMETHODIMP cAIDeath::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (m_DeathCause == kAIDC_Invalid)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }

   SetNotifications(kAICN_GoalChange);           // using base ability handler, which signals action
   *ppGoal = new cAIDieGoal(this);
   (*ppGoal)->priority= kAIP_VeryHigh;
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIDeath::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   // Play the proper death sounds
   if (m_pAI->AccessSoundEnactor())
   {
      m_pAI->AccessSoundEnactor()->HaltCurrent();
      if ((m_pAIState->GetAlertness() < kAIAL_Low) || (m_DeathCause == kAIDC_Knockout))
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_DieSoft);
      else
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_DieLoud);
   }

   cAIMotionAction * pAction = CreateMotionAction();

   // Select the proper death tags
   switch (m_DeathCause)
   {
      case kAIDC_Damage:    pAction->AddTags("Crumple 0,Die 0");      break;
      case kAIDC_Knockout:  pAction->AddTags("Crumple 0,KnockOut 0"); break;
      default:
      {
         Warning(("cAIDeath::SuggestActions: unknown m_DeathCause %d\n", m_DeathCause));
         break;
      }
   }

   pNew->Append(pAction);

   SetNotifications(kAICN_ActionProgress);

   // @DIPPY tell creature to prepare to die, since it needs to shrink phys models etc
   // is this considered a hack? (KJ 6/98)
   CreaturePrepareToDie(m_pAI->GetObjID());

   GhostNotify(m_pAI->GetObjID(),kGhostStDying);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
