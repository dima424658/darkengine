/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////
#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <autolink.h>
#include <traitman.h>

#include <ai.h>
#include <aiapiiai.h>

#include <plyrhdlr.h>
#include <config.h>
#include <matrixs.h>
#include <objtype.h>
#include <command.h>
#include <playrobj.h>
#include <timer.h>
#include <headmove.h>
#include <schema.h>
#include <mtagvals.h>
#include <creatext.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phctrl.h>
#include <phmoapi.h>
#include <physapi.h>
#include <phscrt.h>

#include <invlimb.h>

#include <plyrtype.h>
#include <plycbllm.h>   // PlayerSwitchMode
#include <crattach.h>
#include <matrixc.h>

#include <dpcmelab.h>
#include <dpcmelee.h>
#include <weapon.h>
#include <weaphalo.h>
#include <weapprop.h>
#include <weapreac.h>
#include <weapcb.h>
#include <crwpnapi.h>

#include <rendprop.h>
#include <objpos.h>
#include <plyablty.h>
#include <plyablt_.h>
#include <dpcmel_.h>
#include <dpccret.h>


// traits
#include <dpcparam.h>
#include <dpcplayr.h>
#include <dpctrcst.h>
#include <stimprop.h>   // source scaling
#include <dpcmelpr.h>   // eMeleeType

// psi effects
#include <dpcplcst.h>
//#include <dpcpsapi.h>
//#include <dpcpsipw.h>

// weapon type (through gun system, stupidly)
#include <gunprop.h>

// script messages
#include <scrptapi.h>

// sound
#include <esnd.h>
#include <schema.h>

// weapon listener
#include <linkbase.h>

#include <mvrflags.h>

#include <mprintf.h>

#include <ghostapi.h> // notify ghost about hand weapons

#include <dpcgame.h>

// Must be last header
#include <dbmem.h>

void DPCWeaponIdleCallback(int mode, void *data);

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
   ObjID weapon;

   ObjID target;           // what we're swinging at

   BOOL equip_pending;     // equip motion is playing
   BOOL equipped;          // we are equipped

   BOOL unequip_pending;   // unquip motion is playing

   BOOL swing_ready;       // we are ready to do another swing

   int  cur_swing_type;    // what kind of swing are we powering to?

   BOOL swinging;          // doing a swing

   BOOL powering;          // powering up for a swing
   long powering_time;     // how long have we been powering?

   int  swing_break_2;     // time for med->long transition

   int  weaponSchemaHandle;
} sg_weaponInfo;

///////////////////////////////////////////////////////////////////////////////

static BOOL g_disableWeaponLinkListener = FALSE;

static IIntProperty* g_pMeleeDestroyOnImpact = NULL;

static sPropertyDesc g_meleeDestroyOnImpactPDesc =
{
  "MelDesImpct",
  0,
  NULL, 0, 0,
  { "Weapon", "Melee: Impact Destroy Pct" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////////////////////////////////////////////////////////////////

PhysListenerHandle g_physListenerHandle;

///////////////////////////////////////////////////////////////////////////////

static sg_weaponInfo g_weaponInfo = {OBJ_NULL, OBJ_NULL, FALSE, FALSE, FALSE,
                                 0, FALSE, FALSE, 0, 0, SCH_HANDLE_NULL};

BOOL IsCurrentPlayerArm(ObjID weapon)
{
   cAutoLinkQuery pQuery(g_pCreatureAttachRelation, PlayerArm(), LINKOBJ_WILDCARD);

   while (!pQuery->Done())
   {
      if (weapon == pQuery.GetDest())
         return TRUE;
      pQuery->Next();
   }
   return FALSE;
}

void  DPCWeaponAbilityCallback(int mode, cPlayerAbility **pAbility, void *data)
{
   *pAbility=&g_meleeAbility;
}

void DPCWeaponModeEnterCallback(int mode, void *data)
{
   int weapon_type=0; // @TODO: should get this from somewhere

   if (g_weaponInfo.equip_pending)
   {
      g_weaponInfo.equip_pending = FALSE;
      g_weaponInfo.equipped = TRUE;

      // Attach the weapon to the creature
      CreatureAttachWeapon(PlayerArm(), (int)data, weapon_type);

      // copy corpse links
      AutoAppIPtr(LinkManager);
      AutoAppIPtr(TraitManager);
      IRelation* pCorpseRelation = pLinkManager->GetRelationNamed("Corpse");
      cAutoLinkQuery query(pCorpseRelation, pTraitManager->GetArchetype((int)data), LINKOBJ_WILDCARD);
      while (!query->Done())
      {
         pLinkManager->Add(PlayerArm(), query.GetDest(), pCorpseRelation->GetID());
         query->Next();
      } 
   }
}

void DPCWeaponModeExitCallback(int mode, void *data)
{
   if (g_weaponInfo.unequip_pending)
   {
      g_weaponInfo.unequip_pending = FALSE;
      g_weaponInfo.equipped = FALSE;

      // Detach the weapon from the creature
      CreatureDetachWeapon(PlayerArm(), (int)data);
   }
   
   AutoAppIPtr(DPCPlayer);
   pDPCPlayer->NowNextWeapon(TRUE); // TRUE means do it on NEXT frame
}

void DPCWeaponDestroyCallback(int mode, void *data)
{
   if (g_weaponInfo.unequip_pending)
   {
      g_weaponInfo.unequip_pending = FALSE;
      g_weaponInfo.equipped = FALSE;

      // Detach the weapon from the creature
      CreatureDetachWeapon(PlayerArm(), (int)data);
      AutoAppIPtr(ObjectSystem);
      pObjectSystem->Destroy((int)data);
   }
}

void DPCHitIdleCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   // We have actually returned to the idle position
   g_weaponInfo.swing_ready = TRUE;
};

void DPCWeaponIdleCallback(int mode, void *data)
{
   // Set the hit-idle callback for when we actually reach idle
   AddMotionFlagListener(PlayerArm(), MF_TRIGGER2, DPCHitIdleCallback);

   // Only if we're back to idle after swinging 
   if (g_weaponInfo.swinging)
   {
      // Make the Melee non-physical
      CreatureMakeWeaponNonPhysical(PlayerArm(), (int)data);

      // Clear the weapon's current damage
      ClearWeaponDamageFactor((int)data);

      // Announce that we're finishing our action
      ObjID owner = GetWeaponOwnerObjID((int)data);
      if (owner != OBJ_NULL)
      {
         WeaponEvent(kEndAttack, owner, OBJ_NULL);
//         Warning(("WeapIdleCallback generating EndAttack - is this OK?\n"));
      }

      // Restore the player's head movement
      headmoveSetMouseParams(-1, -1);

      // Set that we're not swinging 
      g_weaponInfo.swinging = FALSE;
   }
}


BOOL DPCWeaponFrameCallback(ulong dt, void *data)
{
   // if powering up Melee, check if crossed power threshold.
   if(g_weaponInfo.powering)
   {
      g_weaponInfo.powering_time+=dt;

      if ((g_weaponInfo.powering_time > g_weaponInfo.swing_break_2) && 
          (g_weaponInfo.cur_swing_type == kPlayerMeleeActionSwingMediumLeft))
      {
         AutoAppIPtr(DPCPlayer);
         if (pDPCPlayer->HasTrait(PlayerObject(), kTraitSmasher))
         {
            PlayerAbortAction();
            PlayerStartActionOfType(kPlayerMeleeActionSwingLong);
            SetWeaponDamageFactor(g_weaponInfo.weapon, 6);

            g_weaponInfo.cur_swing_type = kPlayerMeleeActionSwingLong;
         }
      }
   }
   return FALSE;
}

////////////////////////////////////////

BOOL IsMelee(ObjID weapon)
{
   return g_pMeleeTypeProperty->IsRelevant(weapon);
}
 
////////////////////////////////////////

void MeleeSchemaEndCallback(int hSchema, ObjID schemaID, void *pData)
{
   Assert_(g_weaponInfo.weaponSchemaHandle == hSchema);
   g_weaponInfo.weaponSchemaHandle = SCH_HANDLE_NULL;
}

////////////////////////////////////////

BOOL EquipMelee(ObjID owner, ObjID weapon, int type)
{
   // Set as the active weapon
   SetWeapon(owner, weapon, kWeaponModeMelee);

   // check if player limb model is specified
   Label *pName;

   if(NULL!=(pName=invGetLimbModelName(weapon)))
   {
      g_meleeAbility.SetModelName(pName);
   }
   g_meleeAbility.SetWeaponType(type);

   PlayerHandlerSetAbilityCallback(kDPCPlayerModeMelee, DPCWeaponAbilityCallback, (void *)weapon);

   // Set the callback for when it's done equipping
   PlayerHandlerSetEnterModeCallback(kDPCPlayerModeMelee, DPCWeaponModeEnterCallback, (void *)weapon);
   g_weaponInfo.equip_pending = TRUE;
   g_weaponInfo.unequip_pending = FALSE;

   // Set the callback for when it's idling
   PlayerHandlerSetIdleModeCallback(kDPCPlayerModeMelee, DPCWeaponIdleCallback, (void *)weapon);

   // Set the frame callback
   PlayerHandlerSetFrameCallback(kDPCPlayerModeMelee, DPCWeaponFrameCallback, (void *)weapon);

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kDPCPlayerModeMelee);

   // wow, we are gods
   g_weaponInfo.swing_break_2 = 380;

   g_weaponInfo.swinging = FALSE;

   g_weaponInfo.powering = FALSE;
   g_weaponInfo.swing_ready = FALSE;

   // Activate ESound
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;
   callParams.flags |= SCH_SET_CALLBACK;
   callParams.callback = &MeleeSchemaEndCallback;
   g_weaponInfo.weaponSchemaHandle = ESndPlay(&cTagSet("Event Activate"), weapon, OBJ_NULL, &callParams); 

   // Get the config vars (do we use this in shipping?)
   config_get_int("Melee_break_2", &g_weaponInfo.swing_break_2);

   GhostSetWeapon(owner,weapon,FALSE);
   return TRUE;
} 

////////////////////////////////////////

BOOL UnEquipMeleeCore(ObjID owner, ObjID weapon)
{
   // do the making non-physical etc that would happen when reaching idle
   DPCWeaponIdleCallback(kDPCPlayerModeMelee, (void *)weapon);

   // Set the callback for when it's done unequipping
   PlayerHandlerSetLeaveModeCallback(kDPCPlayerModeMelee, DPCWeaponModeExitCallback, (void *)weapon);
   g_weaponInfo.equip_pending = FALSE;
   g_weaponInfo.unequip_pending = TRUE;

   // Tell the cerebellum to switch modes
   if (isEndingGameMode()) {
      // immediate shutdown so immediate de-equip
      PlayerSwitchMode(kPlayerModeInvalid);
   } else {
      PlayerSwitchMode(kDPCPlayerModeUnarmed);
   }

   AutoAppIPtr(ScriptMan);
   sScrMsg msg(owner, owner, "Unequip", weapon); 
   pScriptMan->SendMessage(&msg); 

   // Kill activate sound (if looping)
   if (g_weaponInfo.weaponSchemaHandle != SCH_HANDLE_NULL)
   {
      SchemaPlayHalt(g_weaponInfo.weaponSchemaHandle);
      g_weaponInfo.weaponSchemaHandle = SCH_HANDLE_NULL;
   }
   // Deactivate sound
   ESndPlay(&cTagSet("Event Deactivate"), weapon, OBJ_NULL);

   GhostSetWeapon(owner,OBJ_NULL,FALSE);
   return TRUE;
}

////////////////////////////////////////

BOOL UnEquipMelee(ObjID owner, ObjID weapon)
{
   // Unset as the active weapon
   if (GetWeaponObjID(owner) == weapon)
   {
      g_disableWeaponLinkListener = TRUE;
      UnSetWeapon(owner);
      g_disableWeaponLinkListener = FALSE;
   }
   return UnEquipMeleeCore(owner, weapon);
}

////////////////////////////////////////

BOOL DestroyMelee(ObjID owner, ObjID weapon)
{
   // do the making non-physical etc that would happen when reaching idle
   DPCWeaponIdleCallback(kDPCPlayerModeMelee, (void *)weapon);

   // Unset as the active weapon
   if (GetWeaponObjID(owner) == weapon)
      UnSetWeapon(owner);

   // Set the callback for when it's done unequipping
   PlayerHandlerSetLeaveModeCallback(kDPCPlayerModeMelee, DPCWeaponDestroyCallback, (void *)weapon);
   g_weaponInfo.equip_pending = FALSE;
   g_weaponInfo.unequip_pending = TRUE;

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeInvalid);

   return TRUE;
}

////////////////////////////////////////

void DPCMakePhysCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   // Make the Melee physical
   int weapon_type=0; // @TODO: should get this from somewhere
   CreatureMakeWeaponPhysical(objID, GetWeaponObjID(PlayerObject()), weapon_type);
}

#define TARG_MAX_DIST    (10)
#define TARG_MAX_DIST_2  (TARG_MAX_DIST * TARG_MAX_DIST)

void DPCFindTargetAndInfo(ObjID owner, ObjID weapon, ObjID *target, int *swing_type)
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
         // get a lot score if you're both close and in-line with our facing.
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
      *swing_type = kPlayerMeleeActionSwingMediumLeft;
   }
   else
   {
      *target = ((cAI *)pBestAI)->GetState()->GetID();
      pBestAI->Release();

      BOOL longSwing = FALSE;
      if (g_pMeleeTypeProperty->IsRelevant(weapon)) {
         int mVar;
         g_pMeleeTypeProperty->Get(weapon, &mVar);
         if (mVar == (int)kMeleeKnuckles) {
            longSwing = TRUE;
         }
      }
      if (longSwing) {
         *swing_type = kPlayerMeleeActionSwingLong;
      }
#if 0
      else if (mx_dot_vec(&best_offset, &orien.vec[1]) < 0)
      // determine if it's the to the left or right of our facing
      {
         *swing_type = kPlayerMeleeActionSwingMediumLeft;
      }
#endif
      else
      {
         *swing_type = kPlayerMeleeActionSwingMediumLeft;
      }
   }
}

void SetMeleeSourceScale(ObjID weapon)
{
   AutoAppIPtr(DPCPlayer);

   // weapon skill
   float stimMod = pDPCPlayer->GetStimMultiplier(weapon);
   
   // lethal weapon trait
   if (pDPCPlayer->HasTrait(PlayerObject(), kTraitLethal))
   {
       stimMod *= GetTraitParams()->m_lethalMult;
   }

   g_pSourceScaleProperty->Set(weapon, stimMod);
}

void ResetMeleeSourceScale(ObjID weapon)
{
   g_pSourceScaleProperty->Set(weapon, 0);
}

void StartMeleeAttack(ObjID owner, ObjID weapon)
{
   // Check if we're not yet equipped or not ready to swing
   if (!g_weaponInfo.equipped || !g_weaponInfo.swing_ready) 
      return;

   if (weapon == OBJ_NULL)
      weapon = GetWeaponObjID(owner);

   if (weapon != OBJ_NULL)
   {
      int   swing_type;
      int destroyPct;

      // Find our target and swing type
      DPCFindTargetAndInfo(owner, weapon, &g_weaponInfo.target, &swing_type);

      // Announce that we're starting to wind up
      WeaponEvent(kStartWindup, owner, g_weaponInfo.target);

      // wind up for swing
      g_weaponInfo.weapon = weapon;
      g_weaponInfo.powering_time = 0;
      g_weaponInfo.powering = TRUE;
      g_weaponInfo.cur_swing_type = swing_type;
      PlayerStartActionOfType(swing_type);

      SetMeleeSourceScale(weapon);

      SetWeaponDamageFactor(weapon, 0);

      AddMotionFlagListener(PlayerArm(), MF_TRIGGER1, DPCMakePhysCallback);

      if (g_pMeleeDestroyOnImpact->Get(weapon, &destroyPct))
      {
         if (RandRange(0, 99)<destroyPct)
         {
             PhysListen(g_physListenerHandle, weapon);      
         }
         else
         {
             PhysUnlisten(g_physListenerHandle, weapon);      
         }
      }
   }
}

void FinishMeleeAction(ObjID owner, ObjID weapon)
{
   if (!g_weaponInfo.equipped) // don't do anything
      return;

   if (!g_weaponInfo.powering)
      return;

   g_weaponInfo.swing_ready = FALSE;

   // Finish swing motion
   PlayerFinishAction();

   if (g_weaponInfo.powering)
   {
      // Announce that we're starting to attack
      switch (g_weaponInfo.cur_swing_type)
      {
         case kPlayerMeleeActionSwingMediumLeft:
         {
            WeaponEvent(kStartAttack, owner, g_weaponInfo.target, kWeaponEventMedium);
            break;
         }
         
         case kPlayerMeleeActionSwingLong:
         {
            WeaponEvent(kStartAttack, owner, g_weaponInfo.target, kWeaponEventLarge);
            break;
         }
      }

      // Slow down the player's head movement
      headmoveSetMouseParams(0x3000, 0);

      // Set that we're swinging
      g_weaponInfo.swinging = TRUE;
   }

   // reset powering globals
   g_weaponInfo.powering_time = 0;
   g_weaponInfo.powering = FALSE;
   g_weaponInfo.cur_swing_type = 0;
}

///////////////////////////////////////////////////////////////////////////////

static void LGAPI DPCMeleePhysicsListener(ObjID objID, const sPhysListenMsg* pMsg, PhysListenerData data)
{
   DestroyMelee(PlayerObject(), objID);
}

////////////////////////////////////////

static void LGAPI WeaponLinkListener(sRelationListenMsg* pMsg, RelationListenerData data)
{
   if (g_disableWeaponLinkListener)
      return;
   // This should only check for *this* player unequipping; otherwise,
   // it'll get invoked every time any *other* player unequips his
   // melee weapon...
   if ((pMsg->type == kListenLinkDeath) && IsPlayerObj(pMsg->link.source))
      UnEquipMeleeCore(pMsg->link.source, pMsg->link.dest);
}

///////////////////////////////////////////////////////////////////////////////

void DPCMeleeInit(void)
{
   g_physListenerHandle = PhysCreateListener("Melee", kCollisionMsg, &DPCMeleePhysicsListener, NULL);
   g_pMeleeDestroyOnImpact = CreateIntProperty(&g_meleeDestroyOnImpactPDesc, kPropertyImplVerySparse);
   g_pWeaponRel->Listen(kListenLinkDeath, WeaponLinkListener, NULL);
}

///////////////////////////////////////////////////////////////////////////////

void DPCMeleeTerm(void)
{
   PhysDestroyListener(g_physListenerHandle);
   SafeRelease(g_pMeleeDestroyOnImpact);
}








