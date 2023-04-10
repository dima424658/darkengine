/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapreac.cpp,v 1.29 1998/10/22 22:27:41 CCAROLLO Exp $
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
#include <weapreac.h>
#include <weapcb.h>

#include <dmgmodel.h>
#include <dmgbase.h>

#include <reaction.h>
#include <reacbase.h>
#include <stimbase.h>

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

#include <aiapi.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

extern int g_CollisionObj1;     // holy crap this is a hack
extern int g_CollisionObj2;
extern int g_CollisionSubmod1;
extern int g_CollisionSubmod2;

////////////////////////////////////////////////////////////////////////////////

static ReactionID create_weapon_hit(IReactions* pReactions);
static ReactionID create_weapon_block(IReactions* pReactions);

static eDamageResult LGAPI WeaponDamageListener(const sDamageMsg *msg, tDamageCallbackData data);

////////////////////////////////////////////////////////////////////////////////

void InitWeaponReactions()
{
   AutoAppIPtr(Reactions);

   create_weapon_hit(pReactions);
   create_weapon_block(pReactions);
}

void InitDamageListener()
{
   AutoAppIPtr(DamageModel);
   pDamageModel->Listen(kDamageMsgImpact | kDamageMsgDamage | kDamageMsgSlay, WeaponDamageListener, NULL);
}

////////////////////////////////////////////////////////////////////////////////

static eReactionResult LGAPI weapon_hit_func(sReactionEvent* event,
                                             const sReactionParam* param,
                                             tReactionFuncData )
{
   ObjID hitee  = param->obj[0];
   ObjID weapon = param->obj[1];

   if ((weapon == OBJ_NULL) || !IsWeaponSwinging(weapon))
      return kReactionNormal;

   if (hitee == PlayerObject())  // Player
   {

      cPhysModel* pModel = g_PhysModels.GetActive(weapon); 
      if (!pModel)
         return kReactionNormal; 

      mxs_vector velocity = pModel->GetVelocity(0); 


      mxs_matrix obj_frame;
      mxs_angvec facing;
      mxs_vector rel_vel;
      PhysGetModRotation(hitee, &facing);
      mx_ang2mat(&obj_frame, &facing);
      mx_mat_tmul_vec(&rel_vel, &obj_frame, &velocity);

      // @TODO: "shove" the player a bit in the direction of weapon momentum

      // do "head smack"
      PlayerMotionSetOffset(PLAYER_HEAD, &rel_vel);

   }

   return kReactionNormal;
}

static sReactionDesc weapon_hit_desc =
{
   REACTION_WEAPON_HIT,
   "Weapon Hit",
   NO_REACTION_PARAM,
   kReactionHasDirectObj|kReactionHasIndirectObj
};


static ReactionID create_weapon_hit(IReactions* pReactions)
{
   return pReactions->Add(&weapon_hit_desc, &weapon_hit_func, NULL);
}

////////////////////////////////////////

static eReactionResult LGAPI weapon_block_func(sReactionEvent* event,
                                             const sReactionParam* param,
                                             tReactionFuncData )
{
   return kReactionNormal; 
}


static sReactionDesc weapon_block_desc =
{
   REACTION_WEAPON_BLOCK,
   "Weapon Block",
   NO_REACTION_PARAM,
   kReactionHasDirectObj|kReactionHasIndirectObj
};


static ReactionID create_weapon_block(IReactions* pReactions)
{
   return pReactions->Add(&weapon_block_desc, &weapon_block_func, NULL);
}

////////////////////////////////////////////////////////////////////////////////

static eDamageResult LGAPI WeaponDamageListener(const sDamageMsg *msg, tDamageCallbackData data)
{
#if 0
   switch (msg->kind)
   {
      case kDamageMsgImpact:
      {
         mprintf("impact (%d)\n", msg->victim);
         mprintf("  mag = %g\n", msg->data.impact->magnitude);
         break;
      }

      case kDamageMsgDamage:
      {
         mprintf("damage (%d)\n", msg->victim);
         mprintf("  amt = %d\n", msg->data.damage->amount);
         break;
      }
   }
#endif
   return kDamageNoOpinion;
}







