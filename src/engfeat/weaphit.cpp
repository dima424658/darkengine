/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weaphit.cpp,v 1.7 2000/03/20 13:51:46 bfarquha Exp $
//
// weapon (hand-to-hand-combat) reaction routines
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <playrobj.h>
#include <objtype.h>
#include <label.h>

#include <drkwswd.h>
#include <plycbllm.h>

#include <weapon.h>
#include <weaphalo.h>
#include <weapprop.h>
#include <weaphit.h>
#include <weapcb.h>

#include <dmgmodel.h>
#include <dmgbase.h>

#include <crattach.h>
#include <creatext.h>
#include <plycbllm.h>

#include <phmods.h>
#include <phmod.h>
#include <phcore.h>
#include <phclsn.h>   // hack hack
#include <collprop.h> // hack hack

#include <physapi.h>
#include <phmoapi.h>

// for attack obj
#include <contag8r.h>
#include <ctactbas.h>

#include <aiapi.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

extern int g_CollisionObj1;     // holy crap this is a hack
extern int g_CollisionObj2;
extern int g_CollisionSubmod1;
extern int g_CollisionSubmod2;



////////////////////////////////////////////////////////////////////////////////


BOOL HandleWeaponHit(ObjID hitee, ObjID weapon)
{
   ObjID owner = GetWeaponOwnerObjID(weapon);

   cPhysModel  *pModel;
   mxs_vector   velocity;
   mxs_vector   location;

#ifdef THIEF
   if (WeaponHaloIsBlocking(hitee))
   {
      int halo_id;
      int hitee_weapon;

      WeaponGetBlockingHalo(hitee, &halo_id);

      hitee_weapon = GetWeaponObjID(hitee);

      if (hitee_weapon == OBJ_NULL)
      {
         Warning(("Halo-owning object doesn't have a weapon?\n"));
         return TRUE;
      }

      // check if the quadrant is permissable

#if 0 // wsf: uh, all this calculation, and none of it is being used???
      mxs_vector sub_pos;
      mxs_real dist;

      if (g_CollisionObj1 == weapon)
         PhysGetSubModLocation(weapon, g_CollisionSubmod1, &sub_pos);
      else
         PhysGetSubModLocation(weapon, g_CollisionSubmod2, &sub_pos);

      dist = WeaponHaloQuadDist(hitee, halo_id, sub_pos);
#endif

      g_CollisionSubmod1 = g_CollisionSubmod2 = 0;

      return HandleWeaponBlock(hitee, hitee_weapon, owner, weapon);
   }
#endif

   if (owner == hitee)
      return FALSE;

   pModel = g_PhysModels.GetActive(weapon);
   if (pModel == NULL)
      return TRUE;

   // check that hit isn't between an AI's weapon and a non-player AI, since
   // resolving this with a damage filter later is too late.


   mx_copy_vec(&velocity, &pModel->GetVelocity(0));
   mx_copy_vec(&location, &pModel->GetLocationVec(0));

   #ifndef SHIP
   if (config_is_defined("WeaponSpew"))
   {
      mprintf("  velocity: %g %g %g\n", velocity.x, velocity.y, velocity.z);
      mprintf("  location: %g %g %g\n", location.x, location.y, location.z);
   }
   #endif

   WeaponEvent(kHitEvent, owner, hitee);

   return TRUE;
}


////////////////////////////////////////

#ifdef THIEF

BOOL ReactToInitialHaloHit(ObjID owner, ObjID weapon, int submod)
{
   if (WeaponHaloInflated(weapon))
   {
      int halo_id;

      WeaponHaloDeflate(owner, weapon);
      WeaponHaloSetBlock(owner, weapon, submod);

      WeaponGetBlockingHalo(owner, &halo_id);
      if (owner==PlayerObject())
      {
         SetBlockMotionType(halo_id);
         PlayerStartActionOfType(kPlayerSwordActionBlockDirected);
      }
      else
      {
         AutoAppIPtr(AIManager);
         IAI *pAI=pAIManager->GetAI(owner);
         pAI->NotifyGameEvent((void *)halo_id);  // woo woo
         SafeRelease(pAI);
      }

      int coll_type;

      if (ObjGetCollisionType(weapon, &coll_type))
      {
         coll_type |= COLLISION_NO_SOUND;
         ObjSetCollisionType(weapon, coll_type);
      }
   }
   return FALSE;
}

#endif

BOOL HandleWeaponBlock(ObjID owner1, ObjID weapon1, ObjID owner2, ObjID weapon2)
{
   int         damage_factor_1;
   int         damage_factor_2;
   cPhysModel *pModel1;
   cPhysModel *pModel2;
   mxs_vector  location;

   int submod1;
   int submod2;

   if (weapon1 == g_CollisionObj1)
   {
      submod1 = g_CollisionSubmod1;
      submod2 = g_CollisionSubmod2;
   }
   else
   {
      submod1 = g_CollisionSubmod2;
      submod2 = g_CollisionSubmod1;
   }

#ifdef THIEF
   // Check if the block is actually a halo hit
   BOOL halo1 = WeaponSubmodIsHalo(weapon1, submod1);
   BOOL halo2 = WeaponSubmodIsHalo(weapon2, submod2);

   if (halo1 && halo2)
      return TRUE;
   else if (halo1)
      return ReactToInitialHaloHit(owner1,weapon1,submod1);
   else if (halo2)
      return ReactToInitialHaloHit(owner2,weapon2,submod2);
#endif

   int coll_type;

   if (ObjGetCollisionType(weapon1, &coll_type))
   {
      coll_type &= ~COLLISION_NO_SOUND;
      ObjSetCollisionType(weapon1, coll_type);
   }

   if (ObjGetCollisionType(weapon2, &coll_type))
   {
      coll_type &= ~COLLISION_NO_SOUND;
      ObjSetCollisionType(weapon2, coll_type);
   }

   // Get their physics models
   pModel1 = g_PhysModels.GetActive(weapon1);
   pModel2 = g_PhysModels.GetActive(weapon2);
   if ((pModel1 == NULL) || (pModel2 == NULL))
      return TRUE;

   // Figure the collision location (approx)
   mx_copy_vec(&location, &pModel1->GetLocationVec(0));
   mx_addeq_vec(&location, &pModel2->GetLocationVec(0));
   mx_scaleeq_vec(&location, 0.5);

   // Get both damage factors
   damage_factor_1 = GetWeaponDamage(weapon1);
   damage_factor_2 = GetWeaponDamage(weapon2);

   #ifndef SHIP
   if (config_is_defined("WeaponSpew"))
   {
      mprintf("%d (owner %d) hits %d (owner %d)\n", weapon1, owner1, weapon2, owner2);
      mprintf("  damage factors = %d %d\n", damage_factor_1, damage_factor_2);
   }
   #endif


   // @TODO: determine direction and momentum of each object

   WeaponEvent(kBlockEvent, owner1, owner2);

   return TRUE;
}

////////////////////////////////////////

typedef cContactPropagator cGator;
#define GATOR_FLAGS (cGator::kNoDefer|cGator::kPhysical)

EXTERN BOOL ResolveWeaponAttack(ObjID culprit, ObjID victim, float magnitude, const struct sChainedEvent* cause)
{
   BOOL hit = FALSE;

   // It don't mean a thing...
   int swing = 0;

   if (culprit != OBJ_NULL)
      swing = IsWeaponSwinging(culprit);

   // I'm a swinging weapon, woo hoo!
   if (swing > 0)
   {
      //      mprintf("%d got hit by weapon %d\n",victim,culprit);
      // Check to see if the victim is a weapon too
      ObjID victim_owner = GetWeaponOwnerObjID(victim);
      ObjID owner = GetWeaponOwnerObjID(culprit);
      if (victim_owner != OBJ_NULL)
      {
         hit = HandleWeaponBlock(owner,culprit,victim_owner,victim);
         WeaponPostHit(culprit,victim,victim_owner);
      }
      else
         hit = HandleWeaponHit(victim,culprit);

      if (hit)
      {
         static ulong event_vec[] = { 0, kPropagateSwingLow, kPropagateSwingMed, kPropagateSwingHigh, kPropagateSwingBlock };

         ulong event = event_vec[swing];
         cGator::gpGator->DoPropagationEvent(event,culprit,victim,magnitude,cause,GATOR_FLAGS);
      }

      WeaponPostHit(victim,culprit,owner);
   }

   return hit;
}

////////////////////////////////////////

EXTERN void WeaponPostHit(ObjID hitee, ObjID weapon, ObjID owner)
{
   if (owner == OBJ_NULL)
      owner = GetWeaponOwnerObjID(owner);

   // Abort their motions
   if (owner == PlayerObject() || owner == PlayerArm())
   {
// wsf: it's really torturesome to have to keep re-hitting the block key. Besides, the
// arm is still positioned to block, but it doesn't really block after this, so let's
// just not do this.
// Unfortunately, this is a general function applied to all weapon hits, not just sword blocking.
#if 1
      CreatureMakeWeaponNonPhysical(PlayerArm(), weapon);
      CreatureAbortWeaponMotion(PlayerArm(), weapon);
#endif
   }
   else
   {
      CreatureMakeWeaponNonPhysical(owner, weapon);
      if (hitee != OBJ_NULL)
         CreatureAbortWeaponMotion(owner, weapon);
   }


}




