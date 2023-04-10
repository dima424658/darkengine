/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

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

#include <dpcaibit.h>
#include <dpcaicha.h>
#include <dpcaidga.h>
#include <dpcaipr.h>

// Must be last header
#include <dbmem.h>

//////////////////////////////////////////////////////////////////////////////

void DPCAIInitDogCombatAbility(void)
{
}

void DPCAITermDogCombatAbility(void)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDogCombat
//

cAIDogCombat::cAIDogCombat():
   m_leapTimer(1000, 2000)
{
}

///////////////////////////////////////

cAIDogCombat::~cAIDogCombat()
{
   ClearNotifications(kAICN_Collision);
}

///////////////////////////////////////
STDMETHODIMP_(void) cAIDogCombat::Init()
{
   cAICombat::Init();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDogCombat::OnSimStart()
{
   sAIDogCombatParams* pParams = DPCAIGetDogCombatParams(GetID());
   m_leapTimer.Set(pParams->m_minLeapTime, pParams->m_maxLeapTime);
   m_state    = kAIDogCombatNothing;
   m_biteDist = pParams->m_biteDist;
   m_leapDist = pParams->m_leapDist;
   cAICombat::OnSimStart();
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIDogCombat::GetName()
{
   return "DogCombat Ability";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDogCombat::Save(ITagFile * pTagFile)
{
   // @TODO: 
   return TRUE;   
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDogCombat::Load(ITagFile * pTagFile)
{
   // @TODO:
  return TRUE;   
}

///////////////////////////////////////

void cAIDogCombat::SetState(eAIDogCombatState state)
{
	switch (state)
   {
      case kAIDogCombatCharge:
      {
         SetNotifications(kAICN_Collision);
         m_leapTimer.Reset();
         break;
      }
      default:
      {
         ClearNotifications(kAICN_Collision);
      }
   }
   SignalAction();
   m_state = state;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDogCombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAICombat::OnGoalChange(pPrevious, pGoal);
   if (InControl())
   {
      SetState(kAIDogCombatCharge);
   }
} 

///////////////////////////////////////
STDMETHODIMP_(void) cAIDogCombat::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);
   if (IsOwn(pAction) && (pAction->GetResult() >= kAIR_Success))
   {
      switch (pAction->GetType())
      {
         case kAIAT_Bite:
         {
            SetState(kAIDogCombatCharge);
            break;
         }
         case kAIAT_Charge:
         {
            if (((cAIChargeAction*)pAction)->ToObject())
            {
               // decide whether or not to leap
               cMxsVector dest;
               GetObjLocation(GetTarget(), &dest);
               float targDistSq = m_pAIState->DistSq(dest);
               if (PhysObjOnGround(m_pAIState->GetID()) && (m_leapTimer.Expired() && (targDistSq<=(m_leapDist*m_leapDist))))
               {
                  SetState(kAIDogCombatLeap);
               }
            }
            else
            {
               SetState(kAIDogCombatCharge);
            }
            break;
         }
         default:
         {
            Warning(("cAIDogCombat: unexpected action type %d\n", pAction->GetType()));
         }
      }
   }
}

////////////////////////////////////////////////

STDMETHODIMP_(void) cAIDogCombat::OnCollision(const sPhysListenMsg* pMsg)
{
   if (m_state == kAIDogCombatCharge)
   {
      if (pMsg->collNormal.z == 1)
      {
         return;
      }
      SetState(kAIDogCombatRandom);
   }
}

////////////////////////////////////////////////

STDMETHODIMP cAIDogCombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   switch (m_state)
   {
      case kAIDogCombatCharge:
      {
         cAIChargeAction* pChargeAction = new cAIChargeAction(this, 0);
         if (pChargeAction != NULL)
         {
            // @TODO: propertize re-path period
            pChargeAction->Set(GetTarget(), kAIT_1Sec);
            pNew->Append(pChargeAction);
         }
         break;
      }
      
      case kAIDogCombatRandom:
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
         break;
      }
      
      case kAIDogCombatLeap:
      {
         cAIBiteAction * pBiteAction = new cAIBiteAction(this, 0);
         if (pBiteAction != NULL)
         {
            AutoAppIPtr(ObjectSystem);
            sAIDogCombatParams* pParams = DPCAIGetDogCombatParams(GetID());
            pBiteAction->Set(GetTarget(), pParams->m_biteDist, pObjectSystem->GetObjectNamed(pParams->m_stimulus), pParams->m_intensity,
                             pParams->m_leapSpeedX, pParams->m_leapSpeedZ);
            pNew->Append(pBiteAction);
         }
         break;
      }
   }
   return S_OK;
}
