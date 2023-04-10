/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigra.cpp,v 1.5 1999/06/16 16:47:00 JON Exp $
//

#include <lg.h>

#include <aiactloc.h>
#include <aiapiiai.h>
#include <aiapipth.h>
#include <aidebug.h>
#include <aipathdb.h>
#include <aiutils.h>

#include <appagg.h>
#include <dmgmodel.h>
#include <iobjsys.h>
#include <phlisbas.h>
#include <physapi.h>
#include <speech.h>

#include <shkaibit.h>
#include <shkaicha.h>
#include <shkaigra.h>
#include <shkaipr.h>

// Must be last header
#include <dbmem.h>

//////////////////////////////////////////////////////////////////////////////

void ShockAIInitGrubCombatAbility(void)
{
}

void ShockAITermGrubCombatAbility(void)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGrubCombat
//

cAIGrubCombat::cAIGrubCombat():
   m_leapTimer(1000, 2000)
{
}

///////////////////////////////////////

cAIGrubCombat::~cAIGrubCombat()
{
   ClearNotifications(kAICN_Collision);
}

///////////////////////////////////////
STDMETHODIMP_(void) cAIGrubCombat::Init()
{
   cAICombat::Init();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIGrubCombat::OnSimStart()
{
   sAIGrubCombatParams* pParams = ShockAIGetGrubCombatParams(GetID());
   m_leapTimer.Set(pParams->m_minLeapTime, pParams->m_maxLeapTime);
   m_state = kAIGrubCombatNothing;
   m_biteDist = pParams->m_biteDist;
   m_leapDist = pParams->m_leapDist;
   cAICombat::OnSimStart();
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIGrubCombat::GetName()
{
   return "GrubCombat Ability";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIGrubCombat::Save(ITagFile * pTagFile)
{
   // @TODO: 
   return TRUE;   
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIGrubCombat::Load(ITagFile * pTagFile)
{
   // @TODO:
  return TRUE;   
}

///////////////////////////////////////

void cAIGrubCombat::SetState(eAIGrubCombatState state)
{
	switch (state)
   {
   case kAIGrubCombatCharge:
      SetNotifications(kAICN_Collision);
      m_leapTimer.Reset();
      break;
   default:
      ClearNotifications(kAICN_Collision);
   }
   SignalAction();
   m_state = state;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIGrubCombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAICombat::OnGoalChange(pPrevious, pGoal);
   if (InControl())
      SetState(kAIGrubCombatCharge);
} 

///////////////////////////////////////

STDMETHODIMP_(void) cAIGrubCombat::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);
   if (IsOwn(pAction) && (pAction->GetResult()>=kAIR_Success))
   {
      switch (pAction->GetType())
      {
      case kAIAT_Bite:
         SetState(kAIGrubCombatCharge);
         break;
      case kAIAT_Charge:
         if (((cAIChargeAction*)pAction)->ToObject())
         {
            // decide whether or not to leap
            cMxsVector dest;
            float targDistSq;
            
            GetObjLocation(GetTarget(), &dest);
            targDistSq = m_pAIState->DistSq(dest);
            if (PhysObjOnGround(m_pAIState->GetID()) && (m_leapTimer.Expired() && (targDistSq<=(m_leapDist*m_leapDist))))
               SetState(kAIGrubCombatLeap);
         }
         else
            SetState(kAIGrubCombatCharge);
         break;
      default:
         Warning(("cAIGrubCombat: unexpected action type %d\n", pAction->GetType()));
      }
   }
}

////////////////////////////////////////////////

STDMETHODIMP_(void) cAIGrubCombat::OnCollision(const sPhysListenMsg* pMsg)
{
   if (m_state == kAIGrubCombatCharge)
   {
      if (pMsg->collNormal.z == 1)
         return;
      SetState(kAIGrubCombatRandom);
   }
}

////////////////////////////////////////////////

STDMETHODIMP cAIGrubCombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   switch (m_state)
   {
   case kAIGrubCombatCharge:
      {
         cAIChargeAction* pChargeAction = new cAIChargeAction(this, 0);
         if (pChargeAction != NULL)
         {
            // @TODO: propertize re-path period
            pChargeAction->Set(GetTarget(), kAIT_1Sec);
            pNew->Append(pChargeAction);
         }
      }
      break;
   case kAIGrubCombatRandom:
      {
         cAIChargeAction* pChargeAction = new cAIChargeAction(this, 0);
         if (pChargeAction != NULL)
         {
            const mxs_vector& aiLoc = *m_pAIState->GetLocation();
            floatang angle;
            mxs_vector target;

            angle.value = DEGREES(AIRandom(0, 360));
            ProjectFromLocationOnZPlane(aiLoc, 3, angle, &target);
            // @TODO: propertize re-path period
            pChargeAction->Set(target, kAIT_2Sec);
            pNew->Append(pChargeAction);
         }
      }
      break;
   case kAIGrubCombatLeap:
      {
         cAIBiteAction * pBiteAction = new cAIBiteAction(this, 0);
         if (pBiteAction != NULL)
         {
            AutoAppIPtr(ObjectSystem);
            sAIGrubCombatParams* pParams = ShockAIGetGrubCombatParams(GetID());
            pBiteAction->Set(GetTarget(), pParams->m_biteDist, pObjectSystem->GetObjectNamed(pParams->m_stimulus), pParams->m_intensity,
               pParams->m_leapSpeedX, pParams->m_leapSpeedZ);
            pNew->Append(pBiteAction);
         }
      }
      break;
   }
   return S_OK;
}
