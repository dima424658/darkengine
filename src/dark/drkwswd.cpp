/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkwswd.cpp,v 1.39 2000/02/26 15:11:15 adurant Exp $

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <ai.h>
#include <aiapiiai.h>

#include <plyrhdlr.h>
#include <plyrmode.h>
#include <plyrvmot.h>
#include <config.h>
#include <matrixs.h>
#include <objtype.h>
#include <command.h>
#include <playrobj.h>
#include <timer.h>
#include <headmove.h>
#include <schema.h>
#include <drkcret.h>
#include <mtagvals.h>
#include <creatext.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phctrl.h>
#include <phmoapi.h>

#include <invlimb.h>

#include <plyrtype.h>
#include <plycbllm.h>   // PlayerSwitchMode
#include <crattach.h>
#include <matrixc.h>

#include <drkwswd.h>
#include <weapon.h>
#include <weaphalo.h>
#include <weapprop.h>
#include <weapreac.h>
#include <weapcb.h>

#include <rendprop.h>
#include <objpos.h>
#include <plyablty.h>
#include <plyablt_.h>
#include <drkwswd_.h>

#include <mvrflags.h>

#include <mprintf.h>

#include <crwpnapi.h>   // kWeaponModeMelee

// Must be last header
#include <dbmem.h>

// @NOTE: this needs to be kept in synch with weapon type enum in
// weapn script service.
enum eDarkWeaponType
{
   kDWT_Sword,
   kDWT_BlackJack,
};


// sword actually cares about action type
class cPlayerSwordAbility : public cPlayerAbility
{
public:
   cPlayerSwordAbility(const sPlayerAbilityDesc *pDesc=NULL);
   virtual ~cPlayerSwordAbility() {}

   virtual int GetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType);

   virtual void SetupSkill(int skillID);

   virtual BOOL GetSoundSchemaName(int skillID,Label *pName);

   virtual BOOL GetMotionTags(int skillID, cTagSet *pTags);

   ///////

   void SetBlockMotionType(int type) { m_BlockMotionType = type; }

   void SetWeaponType(int wtype) { m_WeaponType=wtype; }
   int  GetWeaponType() const { return m_WeaponType; }

private:

   int m_BlockMotionType;
   int m_WeaponType;
};

static sPlayerAbilityDesc g_SwordAbilityDesc={ {"armsw2"},kDCRTYPE_PlayerLimb,kPSS_Begin,kPSS_NumSkills,g_SwordSkillDesc};
static cPlayerSwordAbility g_SwordAbility(&g_SwordAbilityDesc);

void SetBlockMotionType(int type)
{
   g_SwordAbility.SetBlockMotionType(type);
}

int g_SwordStartActionSkill[kPlayerSwordNumActions] =
{
   kPSS_Block,
   kPSS_BlockDirected,
   kPSS_WindupShort,
   kPSS_WindupMediumLeft,
   kPSS_WindupMediumRight,
   kPSS_WindupLongLeft,
   kPSS_WindupLongRight,
};

cPlayerSwordAbility::cPlayerSwordAbility(const sPlayerAbilityDesc *pDesc)
{
   m_BlockMotionType = -1;

   DefaultInit(pDesc);
}

int cPlayerSwordAbility::GetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType)
{
   if(skillID<0||skillID>=m_Desc.nSkills)
      return kSkillInvalid;

   if(input==kPlyrInput_Start)
   {
      sSkillConnectDesc *pConnect=&m_Desc.pSkills[skillID].connect;
      int nextSkill=pConnect->nextIfStart;
      if(nextSkill&&actType!=kPlayerActionTypeInvalid)
      {
         // get special start action for type
         // XXX this may need to be more sophisticated - check if you would
         // otherwise be idling or something.  Only really made to work for
         // sword swings and blocks.  
         // only call if startaction would otherwise at least have you
         // doing _something_        HACK HACK HACK  KJ 1/98

         // note this leaves nextSkill unchanged unless there is a start action
         nextSkill=g_SwordStartActionSkill[actType];
      }
      return nextSkill;
   } else
   {
      return DefaultGetNextSkillID(skillID, input, actType);
   }
}

void cPlayerSwordAbility::SetupSkill(int skillID)
{
   if ((skillID == kPSS_SwingMediumLeft) || (skillID == kPSS_SwingMediumRight) || 
       (skillID == kPSS_SwingLongLeft) ||  (skillID == kPSS_SwingLongRight))
      PlayerMotionActivate(kMoWeaponSwing);

   DefaultSetupSkill(skillID);
}

BOOL cPlayerSwordAbility::GetSoundSchemaName(int skillID,Label *pName)
{
   if(GetWeaponType()==kDWT_BlackJack)
   {
      if(skillID==kPSS_Begin)
      {
         strcpy(pName->text,"bjack_begin");
         return TRUE;
      } else if(skillID==kPSS_End)
      {
         strcpy(pName->text,"bjack_end");
         return TRUE;
      } else if(skillID==kPSS_SwingShort)
      {
         strcpy(pName->text,"bjack_swing");
         return TRUE;
      }
   }
   return cPlayerAbility::GetSoundSchemaName(skillID, pName);
}

BOOL cPlayerSwordAbility::GetMotionTags(int skillID, cTagSet *pTags)
{
   if (cPlayerAbility::GetMotionTags(skillID, pTags))
   {
      if (skillID == kPSS_BlockDirected)
      {
         switch (m_BlockMotionType)
         {
            case 0: pTags->Add(cTag("Direction", kMTV_high)); break;
            case 1: pTags->Add(cTag("Direction", kMTV_right)); break;
            case 2: pTags->Add(cTag("Direction", kMTV_low)); break;
            case 3: pTags->Add(cTag("Direction", kMTV_left)); break;
            default:
            {
               Warning(("GetMotionType: unknown block motion type %d\n", m_BlockMotionType));
               break;
            }
         }
      }
      if(GetWeaponType()==kDWT_BlackJack)
      {
         pTags->Add(cTag("BlackJack",kMTV_set));
      }
   }
   else
      return FALSE;

   return TRUE;
}

void WeaponIdleCallback(int mode, void *data);

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
   ObjID weapon;

   ObjID target;           // what we're swinging at

   BOOL equip_pending;     // equip motion is playing
   BOOL equipped;          // we are equipped

   BOOL unequip_pending;   // unquip motion is playing

   BOOL attack_pending;
   BOOL block_pending;

   BOOL swing_ready;       // we are ready to do another swing

   int  cur_swing_type;    // what kind of swing are we powering to?

   BOOL swinging;          // doing a swing
   BOOL blocking;          // doing a block

   BOOL powering;          // powering up for a swing
   long powering_time;     // how long have we been powering?

   int  swing_to_overhead_ms;  
   int  swing_force_finish_ms;
   int  swing_low_wobble_ms;
   int  swing_high_wobble_ms;
   int  swing_small_time;
} sWeaponInfo;

static sWeaponInfo gWeaponInfo = {0, 0, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, -1, FALSE, FALSE, FALSE,
                                  0, 0, 0, 0, 0, 0};

void  WeaponAbilityCallback(int mode, cPlayerAbility **pAbility, void *data)
{
   *pAbility=&g_SwordAbility;
}

void  WeaponModeEnterCallback(int mode, void *data)
{
   int weapon_type=0; // @TODO: should get this from somewhere

   if (gWeaponInfo.equip_pending)
   {
      gWeaponInfo.equip_pending = FALSE;
      gWeaponInfo.equipped = TRUE;

      // Attach the weapon to the creature
      CreatureAttachWeapon(PlayerArm(), (int)data, weapon_type);
   }
}

void WeaponModeExitCallback(int mode, void *data)
{
   if (gWeaponInfo.unequip_pending)
   {
      gWeaponInfo.unequip_pending = FALSE;
      gWeaponInfo.equipped = FALSE;

      // Detach the weapon from the creature
      CreatureDetachWeapon(PlayerArm(), (int)data);
   }
}

void HitIdleCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   // We have actually returned to the idle position
   gWeaponInfo.swing_ready = TRUE;
};

void WeaponIdleCallback(int mode, void *data)
{
   // Set the hit-idle callback for when we actually reach idle
   AddMotionFlagListener(PlayerArm(), MF_TRIGGER2, HitIdleCallback);

   // Only if we're back to idle after swinging or blocking
   if (gWeaponInfo.swinging || gWeaponInfo.blocking)
   {
      // Make the sword non-physical
      CreatureMakeWeaponNonPhysical(PlayerArm(), (int)data);

      // Clear the weapon's current damage
      ClearWeaponDamageFactor((int)data);

      // Announce that we're finishing our action
      ObjID owner = GetWeaponOwnerObjID((int)data);
      if (owner != OBJ_NULL)
      {
         if (gWeaponInfo.blocking)
            WeaponEvent(kEndBlock, owner, OBJ_NULL);
         else
            WeaponEvent(kEndAttack, owner, OBJ_NULL);
      }

      // Restore the player's head movement
      headmoveSetMouseParams(-1, -1);

      // Set that we're not swinging or blocking
      gWeaponInfo.swinging = FALSE;
      gWeaponInfo.blocking = FALSE;
   }
}

static BOOL  g_Wobbling;
static float g_WobbleTime;
static float g_Wobble;
static float g_WobbleRate;

static void WobbleArmFilter(mxs_vector *loc, mxs_matrix *orient)
{
   if (g_Wobbling)
   {
      mxs_matrix new_orient;
      mxs_vector unit;
      mxs_vector x_vec;
      mxs_vector y_vec;

      mx_mk_vec(&unit, 0, g_Wobble, 0);
      mx_mat_mul_vec(&x_vec, orient, &unit);
      mx_mk_vec(&unit, 0, 0, g_Wobble);
      mx_mat_mul_vec(&y_vec, orient, &unit);

      mx_add_vec(&new_orient.vec[0], &orient->vec[0], &x_vec);
      mx_addeq_vec(&new_orient.vec[0], &y_vec);

      mx_cross_vec(&new_orient.vec[1], &orient->vec[2], &new_orient.vec[0]);
      mx_cross_vec(&new_orient.vec[2], &new_orient.vec[0], &new_orient.vec[1]);

      for (int i=0; i<3; i++)
      {
         if (mx_mag2_vec(&new_orient.vec[i]) < 0.0001)
            return;
         
         mx_normeq_vec(&new_orient.vec[i]);
      }

      mx_copy_mat(orient, &new_orient);
   }
}

void UpdateArmWobble(ulong dt, float magnitude)
{
   if (!g_Wobbling)
   {
      // need to init our wobbling
      g_Wobbling = TRUE;
      g_WobbleTime = 0.0;

      SetPlayerArmFilter(WobbleArmFilter);

      g_WobbleRate = 0.002;
   }

   float time = g_WobbleTime;

   g_WobbleTime += dt;
   g_Wobble = (sin(g_WobbleRate * g_WobbleTime)) * 0.02 * magnitude;
}

BOOL WeaponFrameCallback(ulong dt, void *data)
{

	//if swimming, abort.  Do we want to block other modes?
	// AMSD 
   if ((gWeaponInfo.powering) && (GetPlayerMode()==kPM_Swim))
   {
	   FinishWeaponAction(GetWeaponOwnerObjID(gWeaponInfo.weapon), gWeaponInfo.weapon);
	   return FALSE;
   }
	   	   	   
   // if powering up sword, check if crossed power threshold.
   if(g_SwordAbility.GetWeaponType()==kDWT_Sword && gWeaponInfo.powering)
   {
      ulong prev_time = gWeaponInfo.powering_time;
      gWeaponInfo.powering_time+=dt;

      if ((prev_time < gWeaponInfo.swing_small_time) &&
          (gWeaponInfo.powering_time >= gWeaponInfo.swing_small_time))
      {
         const Label garsword = {"garsword"};

         SchemaPlay(&garsword, NULL);
      }

      if ((gWeaponInfo.powering_time > gWeaponInfo.swing_to_overhead_ms) && 
          (gWeaponInfo.cur_swing_type == kPlayerSwordActionSwingMediumLeft))

      {
         ClearPlayerArmFilter();
         g_Wobbling = FALSE;

         PlayerAbortAction();
         PlayerStartActionOfType(kPlayerSwordActionSwingLongLeft);
         SetWeaponDamageFactor(gWeaponInfo.weapon, 6);

         gWeaponInfo.cur_swing_type = kPlayerSwordActionSwingLongLeft;
      }

      if ((gWeaponInfo.powering_time > gWeaponInfo.swing_to_overhead_ms) &&
          (gWeaponInfo.cur_swing_type == kPlayerSwordActionSwingMediumRight))
      {
         ClearPlayerArmFilter();
         g_Wobbling = FALSE;

         PlayerAbortAction();
         PlayerStartActionOfType(kPlayerSwordActionSwingLongRight);
         SetWeaponDamageFactor(gWeaponInfo.weapon, 6);

         gWeaponInfo.cur_swing_type = kPlayerSwordActionSwingLongRight;
      }

      if (gWeaponInfo.powering_time > gWeaponInfo.swing_force_finish_ms)
      {
         ClearPlayerArmFilter();
         g_Wobbling = FALSE;

         FinishWeaponAction(GetWeaponOwnerObjID(gWeaponInfo.weapon), gWeaponInfo.weapon);
      }

      if ((gWeaponInfo.powering_time > gWeaponInfo.swing_low_wobble_ms) &&
          ((gWeaponInfo.cur_swing_type == kPlayerSwordActionSwingMediumLeft) ||
           (gWeaponInfo.cur_swing_type == kPlayerSwordActionSwingMediumRight)))
      {
         UpdateArmWobble(dt, 1.0);
      }

      if ((gWeaponInfo.powering_time > gWeaponInfo.swing_high_wobble_ms) &&
          ((gWeaponInfo.cur_swing_type == kPlayerSwordActionSwingLongLeft) ||
           (gWeaponInfo.cur_swing_type == kPlayerSwordActionSwingLongRight)))
      {
         UpdateArmWobble(dt, 1.0);
      }

   }
   else
   {
      UpdateArmWobble(dt * 0.6, 2.0);
   }

   return FALSE;
}

////////////////////////////////////////

BOOL  EquipWeapon(ObjID owner, ObjID weapon, int type)
{
   // Set as the active weapon
   SetWeapon(owner, weapon, kWeaponModeMelee);

   // check if player limb model is specified
   Label *pName;

   if(NULL!=(pName=invGetLimbModelName(weapon)))
   {
      g_SwordAbility.SetModelName(pName);
   }
   g_SwordAbility.SetWeaponType(type);

   PlayerHandlerSetAbilityCallback(kPlayerModeSword, WeaponAbilityCallback, (void *)weapon);

   // Set the callback for when it's done equipping
   PlayerHandlerSetEnterModeCallback(kPlayerModeSword, WeaponModeEnterCallback, (void *)weapon);
   gWeaponInfo.equip_pending = TRUE;
   gWeaponInfo.unequip_pending = FALSE;

   // Set the callback for when it's idling
   PlayerHandlerSetIdleModeCallback(kPlayerModeSword, WeaponIdleCallback, (void *)weapon);

   // Set the frame callback
   PlayerHandlerSetFrameCallback(kPlayerModeSword, WeaponFrameCallback, (void *)weapon);

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeSword);

   gWeaponInfo.swing_to_overhead_ms = 1200;
   gWeaponInfo.swing_force_finish_ms = 8000;
   gWeaponInfo.swing_low_wobble_ms = 200;
   gWeaponInfo.swing_high_wobble_ms = 1200;
   gWeaponInfo.swing_small_time = 650;

   gWeaponInfo.swinging = FALSE;
   gWeaponInfo.blocking = FALSE;

   gWeaponInfo.powering = FALSE;
   gWeaponInfo.swing_ready = FALSE;

   // Get the config vars
#ifdef PLAYTEST
   config_get_int("sword_to_overhead", &gWeaponInfo.swing_to_overhead_ms);
   config_get_int("sword_force_finish", &gWeaponInfo.swing_force_finish_ms);
   config_get_int("sword_low_wobble", &gWeaponInfo.swing_low_wobble_ms);
   config_get_int("sword_high_wobble", &gWeaponInfo.swing_high_wobble_ms);
#endif

   return TRUE;
}

BOOL  UnEquipWeapon(ObjID owner, ObjID weapon)
{
   // do the making non-physical etc that would happen when reaching idle
   WeaponIdleCallback(kPlayerModeSword, (void *)weapon);

   // Unset as the active weapon
   if (GetWeaponObjID(owner) == weapon)
      UnSetWeapon(owner);

   // Set the callback for when it's done unequipping
   PlayerHandlerSetLeaveModeCallback(kPlayerModeSword, WeaponModeExitCallback, (void *)weapon);
   gWeaponInfo.equip_pending = FALSE;
   gWeaponInfo.unequip_pending = TRUE;

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeUnarmed);

   return TRUE;
}

////////////////////////////////////////

BOOL  IsWeaponEquipped(ObjID owner, ObjID weapon)
{
   return ((GetWeaponObjID(owner) == weapon) && gWeaponInfo.equipped);
}

// @TODO: someday, we need a real version of this for things != player in weapon.cpp
BOOL WeaponIsSword(ObjID owner, ObjID weapon)
{
   if (IsWeaponEquipped(owner,weapon))
      if (g_SwordAbility.GetWeaponType()==kDWT_Sword)
         return TRUE;
   return FALSE;
}

 // i suck
BOOL WeaponIsBlackjack(ObjID owner, ObjID weapon)
{
   if (IsWeaponEquipped(owner,weapon))
      if (g_SwordAbility.GetWeaponType()==kDWT_BlackJack)
         return TRUE;
   return FALSE;
}

void MakePhysCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   // Make the sword physical
   int weapon_type=0; // @TODO: should get this from somewhere
   // Switching weapons in mid swing caused the new weapon to get made physical, even
   // though no weapon was physical.  This check should fix that.  AMSD 2/26/00
   if ((!gWeaponInfo.equip_pending) && (!gWeaponInfo.unequip_pending))   
      CreatureMakeWeaponPhysical(objID, GetWeaponObjID(PlayerObject()), weapon_type);
   else
      Warning(("Player(%d) attempted to make melee weapon %s physical but had a pending equip or unequip!\n",
               PlayerObject(),
               ObjWarnName(GetWeaponObjID(PlayerObject()))));
}

void MakeNonPhysCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   CreatureMakeWeaponNonPhysical(objID, GetWeaponObjID(PlayerObject()));
}

#define TARG_MAX_DIST    (10)
#define TARG_MAX_DIST_2  (TARG_MAX_DIST * TARG_MAX_DIST)

void FindTargetAndInfo(ObjID owner, ObjID *target, int *swing_type)
{
   mxs_vector owner_loc;
   mxs_matrix orien;

   owner_loc = ObjPosGet(owner)->loc.vec;
   mx_ang2mat(&orien, &ObjPosGet(owner)->fac);

   //@TODO: Use prox system for this!
   AutoAppIPtr_(AIManager, pAIMan);

   IAI *pAI;
   tAIIter iter;
   mxs_vector ai_loc;
   mxs_real   dist;
   mxs_vector ai_offset;

   IAI       *pBestAI = NULL;
   mxs_real   best_value;
   mxs_vector best_offset;

   pAI = pAIMan->GetFirst(&iter);
   while (pAI)
   {
      ai_loc = ObjPosGet(((cAI *)pAI)->GetState()->GetID())->loc.vec;

      mx_sub_vec(&ai_offset, &ai_loc, &owner_loc);
      dist = mx_mag_vec(&ai_offset);

      if (dist < TARG_MAX_DIST)
      {
         mxs_real offset_proj;
         mxs_real value;

         // the value is the distance, plus the difference between the offset
         // vector and the offset vector's projection onto our facing.  So you
         // get a low score if you're both close and in-line with our facing.
         // low scores are good.

         offset_proj = mx_dot_vec(&orien.vec[0], &ai_offset);

         if (offset_proj > 0)
         {
            value = dist + (offset_proj - dist);
 
            if ((pBestAI == NULL) || (value < best_value))
            {
               SafeRelease(pBestAI);
               pBestAI = pAI;
               pBestAI->AddRef();
               best_value = value;
               best_offset = ai_offset;
            }
         }
      }
      pAI->Release();
      pAI = pAIMan->GetNext(&iter);
   }
   pAIMan->GetDone(&iter);

   if (pBestAI == NULL)
   {
      *target = OBJ_NULL;
      *swing_type = kPlayerSwordActionSwingMediumLeft;
   }
   else
   {
      *target = ((cAI *)pBestAI)->GetState()->GetID();
      pBestAI->Release();

      // determine if it's the to the left or right of our facing
      if (mx_dot_vec(&best_offset, &orien.vec[1]) < 0)
      {
         *swing_type = kPlayerSwordActionSwingMediumLeft;
      }
      else
      {
//         *swing_type = kPlayerSwordActionSwingMediumLeft;
         *swing_type = kPlayerSwordActionSwingMediumRight;
      }
   }
}

void UpdateWeaponAttack()
{
   if (gWeaponInfo.attack_pending && gWeaponInfo.equipped && gWeaponInfo.swing_ready)
   {
      ObjID owner = GetWeaponOwnerObjID(gWeaponInfo.weapon);

      if (owner)
         StartWeaponAttack(owner, gWeaponInfo.weapon);
      gWeaponInfo.attack_pending = FALSE;
   }

   if (gWeaponInfo.block_pending && gWeaponInfo.equipped && gWeaponInfo.swing_ready)
   {
      ObjID owner = GetWeaponOwnerObjID(gWeaponInfo.weapon);

      if (owner)
         StartWeaponBlock(owner, gWeaponInfo.weapon);
      gWeaponInfo.attack_pending = FALSE;
   }
}

void StartWeaponBlock(ObjID owner, ObjID weapon)
{
   // Check if we're not yet equipped or not ready to swing
   if (!gWeaponInfo.equipped || !gWeaponInfo.swing_ready)
   {
      gWeaponInfo.block_pending = TRUE;
      return;
   }

   if (GetPlayerMode() != kPM_Stand)
   {
      if (GetPlayerMode() == kPM_Crouch)
         SetPlayerMode(kPM_Stand);
      else
      {
         gWeaponInfo.block_pending = TRUE;
         return;
      }
   }

   if (weapon == OBJ_NULL)
      weapon = GetWeaponObjID(owner);

   if (weapon != OBJ_NULL)
   {
      PlayerStartActionOfType(kPlayerSwordActionBlock);

      // Inflate the blocking halo
      WeaponHaloInflate(owner, weapon);

      // Announce that we're starting to block
      WeaponEvent(kStartBlock, owner, OBJ_NULL);

      // Make the sword physical
      int weapon_type=0; // @TODO: should get this from somewhere
      CreatureMakeWeaponPhysical(PlayerArm(), weapon, weapon_type);

      // Slow down the player's head movement
      headmoveSetMouseParams(0x3000, 0);

      // Set that we're blocking
      gWeaponInfo.blocking = TRUE;

      // We're not able to swing while blocking
      gWeaponInfo.swing_ready = FALSE;
   }
}

void StartWeaponAttack(ObjID owner, ObjID weapon)
{
   // Check if we're not yet equipped or not ready to swing
   if (!gWeaponInfo.equipped || !gWeaponInfo.swing_ready)
   {
      gWeaponInfo.attack_pending = TRUE;
      return;
   }

   if (GetPlayerMode() != kPM_Stand)
   {
      if (GetPlayerMode() == kPM_Crouch)
         SetPlayerMode(kPM_Stand);
      else
      {
         gWeaponInfo.attack_pending = TRUE;
         return;
      }
   }

   if (weapon == OBJ_NULL)
      weapon = GetWeaponObjID(owner);

   if (weapon != OBJ_NULL)
   {
      int   swing_type;

      // Find our target and swing type
      FindTargetAndInfo(owner, &gWeaponInfo.target, &swing_type);

      if (g_SwordAbility.GetWeaponType() == kDWT_BlackJack)
         swing_type = kPlayerSwordActionSwingShort;

      // Announce that we're starting to wind up
      WeaponEvent(kStartWindup, owner, gWeaponInfo.target);

      // wind up for swing
      gWeaponInfo.weapon = weapon;
      gWeaponInfo.powering_time = 0;
      gWeaponInfo.powering = TRUE;
      gWeaponInfo.cur_swing_type = swing_type;
      PlayerStartActionOfType(swing_type);

      if (g_SwordAbility.GetWeaponType() != kDWT_BlackJack)
         SetWeaponDamageFactor(weapon, 0);

      AddMotionFlagListener(PlayerArm(), MF_TRIGGER1, MakePhysCallback);
      AddMotionFlagListener(PlayerArm(), MF_TRIGGER2, MakeNonPhysCallback);
   }
}

void  FinishWeaponAction(ObjID owner, ObjID weapon)
{
   gWeaponInfo.attack_pending = FALSE;
   gWeaponInfo.block_pending = FALSE;

   if (!gWeaponInfo.equipped) // don't do anything
      return;

   if (!gWeaponInfo.blocking && !gWeaponInfo.powering)
      return;

   ClearPlayerArmFilter();
   g_Wobbling = FALSE;

   gWeaponInfo.swing_ready = FALSE;

   // Finish swing motion
   PlayerFinishAction();
   
   if (gWeaponInfo.blocking)
   {
      WeaponHaloDeflate(owner, weapon);
      CreatureAbortWeaponMotion(owner, weapon);
   }
   
   if (gWeaponInfo.powering)
   {
      if (gWeaponInfo.powering_time<gWeaponInfo.swing_small_time)
         gWeaponInfo.cur_swing_type=kPlayerSwordActionSwingShort;
      
      // Announce that we're starting to attack
      switch (gWeaponInfo.cur_swing_type)
      {
         case kPlayerSwordActionSwingShort:
         {
            WeaponEvent(kStartAttack, owner, gWeaponInfo.target, kWeaponEventSmall);
            break;
         }
         
         case kPlayerSwordActionSwingMediumLeft:
         case kPlayerSwordActionSwingMediumRight:
         {
            WeaponEvent(kStartAttack, owner, gWeaponInfo.target, kWeaponEventMedium);
            break;
         }
         
         case kPlayerSwordActionSwingLongLeft:
         case kPlayerSwordActionSwingLongRight:
         {
            WeaponEvent(kStartAttack, owner, gWeaponInfo.target, kWeaponEventLarge);
            break;
         }
      }
      
      // Slow down the player's head movement
      headmoveSetMouseParams(0x3000, 0);
      
      // Set that we're swinging
      gWeaponInfo.swinging = TRUE;
   }
      
   // reset powering globals
   gWeaponInfo.powering_time = 0;
   gWeaponInfo.powering = FALSE;
   gWeaponInfo.cur_swing_type = 0;
}
