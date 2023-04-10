/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrsht.cpp,v 1.9 2000/02/03 16:36:39 bfarquha Exp $
//
// AI Combat - ranged
//

#include <aicbrsht.h>

#include <appagg.h>
#include <comtools.h>

#include <contain.h>
#include <iobjsys.h>
#include <objdef.h>
#include <phprop.h>
#include <rendprop.h>

#include <aiactrng.h>
#include <aiapisnd.h>
#include <aicbrmpr.h>
#include <aidebug.h>
#include <aiprops.h>
#include <aiprrngd.h>
#include <aisndtyp.h>
#include <aitrginf.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

const float kAIRS_HeadingTolerance = 0.25;

////////////////////////////////////////

void cAIRangedShoot::Reset(void)
{
   projectile = OBJ_NULL;
   proj_link_data = NULL;
}

////////////////////////////////////////

int cAIRangedShoot::SuggestApplicability(void)
{
// wsf: timer causing problems. When we're ready to fire (new ranged combat), the question
// is asked if this mode (long range shoot) wants to continue. It responds 'no' because the
// timer hasn't kicked off yet (still have .7 seconds to go for frogmen - the timer stars when action is
// selected), which makes this mode come to a premature end, and be replaced with frustration.

//   if (m_pOwner->m_FiringDelay.Expired() && GetCurrentLocation()->GetLOF())

   if (GetCurrentLocation()->GetLOF())
      return g_AIRangedCombatModeWeights[AIGetRangedShoot(GetID())->m_rangeApplicability[GetRangeClass()]];
   else
      return 0;
}

////////////////////////////////////////

BOOL cAIRangedShoot::CheckContinuation(void)
{
   sAIRangedShootParams *pParams = AIGetRangedShoot(GetID());

   if ((pParams->m_confirmRange) && (pParams->m_rangeApplicability[GetRangeClass()]==kAIRC_AppNone))
      return FALSE;
   if ((pParams->m_confirmLOF) && !m_pOwner->GetCurrentLocation()->TestLOF())
      return FALSE;
   return TRUE;
}

////////////////////////////////////////

int cAIRangedShoot::SuggestInterrupt(void)
{
   // @TODO: interrupt should look at property & how far off our facing is
   return SuggestApplicability();
}

////////////////////////////////////////

BOOL cAIRangedShoot::CheckPreconditions(void)
{
   m_pOwner->SelectProjectile(&projectile, &proj_link_data, &targeting_location);
   return (projectile != OBJ_NULL);
}

////////////////////////////////////////

cAIAction* cAIRangedShoot::SuggestAction(void)
{
   cAIAction * pAction = NULL;

   Assert_(projectile != OBJ_NULL);
   AIWatch1(Ranged, GetID(), "Launching projectile: %d", projectile);

   // Tell all the other projectiles that they weren't selected
   m_pOwner->RefreshProjectilesProj(projectile);

   // Pull one out of the inventory
   AutoAppIPtr(ContainSys);
   AutoAppIPtr_(ObjectSystem, pObjSys);
   ObjID firing_projectile;

   // We want to delete the object if we run out of ammo, but we also
   // want its link around until we're done creating the action, so
   // we defer deletions until then.
   pObjSys->Lock();

   // Clone it
   firing_projectile = pObjSys->Create(projectile, kObjectConcrete);

   // Remove its physics and make it invisible
   g_pPhysTypeProp->Delete(firing_projectile);
   ObjSetHasRefs(firing_projectile, FALSE);

   // Decrement stack count
   int fire_count;
   int burst_count;

   if (proj_link_data->burst_count > 0)
      burst_count = proj_link_data->burst_count;
   else
      burst_count = 1;

   if (proj_link_data->ammo > 0)
   {
      IIntProperty *pStackCountProp = pContainSys->StackCountProp();
      int count;

      if (pStackCountProp->Get(projectile, &count))
      {
         if (count < burst_count)
            fire_count = count;
         else
            fire_count = burst_count;

         count -= fire_count;

         if (count > 0)
            pStackCountProp->Set(projectile, count);
         else
            pObjSys->Destroy(projectile);
      }
      else
         CriticalMsg("Selected projectile w/o ammo!");
   }
   else
      fire_count = burst_count;

   // @TODO: handle specifying launch point

   cAIAttackRangedAction *pAttackAction = new cAIAttackRangedAction(m_pOwner);

   if (m_pOwner->m_ReactShootTimer.Expired())
   {
      if (GetInternalAI()->AccessSoundEnactor())
         GetInternalAI()->AccessSoundEnactor()->RequestConcept(kAISC_ReactShoot);
      m_pOwner->m_ReactShootTimer.Reset();
   }

   if ((proj_link_data->targeting_method == kTM_StraightLine) ||
      (proj_link_data->targeting_method == kTM_Arcing))
   {
      if (GetCurrentLocation()->GetTargetSubModel()>=0)
         pAttackAction->Set(GetTargetInfo()->id, GetCurrentLocation()->GetTargetSubModel(), firing_projectile, proj_link_data->leads_target,
            proj_link_data->accuracy, fire_count, proj_link_data->launch_joint);
      else
         pAttackAction->Set(GetTargetInfo()->id, firing_projectile, proj_link_data->leads_target,
            proj_link_data->accuracy, fire_count, proj_link_data->launch_joint);
   }
   else
   {
      pAttackAction->Set(targeting_location, firing_projectile, proj_link_data->accuracy,
         fire_count, proj_link_data->launch_joint);
   }
   // Tell the action how fast to rotate and how close is close enough (in heading)
   pAttackAction->SetRotationParams(AIGetRangedShoot(GetID())->m_rotationSpeed, kAIRS_HeadingTolerance);

   pObjSys->Unlock();

#define kRandVar 15
   float randVar = 1.0 + (AIRandom(0, kRandVar * 2) - kRandVar) / 100.0;
   m_pOwner->m_FiringDelay.Set((eAITimerPeriod)(unsigned)(GetRangedCombatProp()->firing_delay * 1000 * randVar));

   return pAttackAction;
}









