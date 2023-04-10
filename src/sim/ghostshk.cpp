/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostshk.cpp,v 1.3 1999/11/19 14:49:30 adurant Exp $
// shock specific ghost functions

#include <comtools.h>
#include <appagg.h>
#include <netman.h>

#include <ghost.h>
#include <ghostapi.h>
#include <ghosttyp.h>
#include <ghostwep.h>
#include <ghostshk.h>
#include <ghostlst.h>

#include <playrobj.h>

#include <shkcret.h>
#include <gunflash.h>
#include <gunapi.h>

#include <shkparam.h>
#include <shkplayr.h>
#include <shktrcst.h>

#include <mmanuver.h>    // for mocap zaniness
#include <mcoord.h>
#include <mclntapi.h>
#include <motdesc.h>
#include <motset.h>
#include <motprop.h>
#include <mschutil.h>
#include <motmngr.h>
#include <mvrflags.h>

#include <esnd.h>
#include <schema.h>

#include <dbmem.h>

////////////////////////////
// weapon swizzling

// -1 is none, else 0-n is a "real" weapon
int  ShockSwizzleWeaponObj(ObjID weap, BOOL charging)
{
   return GhostGetWeaponIdCharging(weap,charging);
}

// attach this weapon to pGR
void ShockDeSwizzleWeaponObj(ObjID ghost, int weap)
{
   sGhostRemote *pGR=GhostGetRemote(ghost);
   GhostAttachWeapon(pGR,weap);
}

///////////////////////////
// player action to schema idx conversion

int ShockPlayerActionToSchemaIdx(ObjID ghost, int mode, int state)
{
#ifdef SHOCK
   int actorType=kShATYPE_Humanoid;
   cMotionSchema *pSchema=NULL;
   cTagDBInput useTags;

   // here we pretty much inline "SetupTags" from mskilset.cpp
#ifdef TRY_AND_DO_THINGS_RIGHT
   IMotor *pMotor=CreatureGetMotorInterface(obj);
   sMotActorTagList *pActorTags=NULL;
   if (pMotor&&ObjGetActorTagList(obj,&pActorTags))
   {
      cTagSet propTagSet;
      propTagSet.FromString(pActorTags->m_TagStrings);
      useTags.AppendTagSet(&propTagSet);
   }
#else
   cTagSet actorTagSet;
   actorTagSet.FromString("Player");
   useTags.AppendTagSet(&actorTagSet);
#endif
   cTagSet actionTagSet;
   char *c_str=NULL;
   switch (state)
   {         // @TBD: also 5, if we want it, hook to game trait thing
      case kGhostStSwinging:
      {
         AutoAppIPtr(ShockPlayer);
         if (pShockPlayer->HasTrait(PlayerObject(), kTraitSmasher))
            c_str="attack, meleecombat, direction 5";
         else
            c_str="attack, meleecombat, direction 0";
         break;
      }
      case kGhostStDying:    c_str="crumple"; break;
      case kGhostStFiring:
         if (mode == kGhostModeCrouch)
            c_str="rangedcombat, crouching";
         else
            c_str="rangedcombat";
         break;
      case kGhostStWounded:  c_str="receivewound"; break;
      default: Warning(("Invalid state in getPlayerSchema %x\n",state)); break;
   }
   if (c_str)
   {
      actionTagSet.FromString(c_str);
      useTags.AppendTagSet(&actionTagSet);
   }
   else
      return kGhostMotSchemaNoCustom;

   if (!g_pMotionDatabase->GetBestMatch(actorType,&useTags,&pSchema))
      return kGhostMotSchemaNoCustom;
   if (_ghost_track_player())
      _ghost_mprintf(("plyschema %d for <%s> from %x (%d mots)\n",pSchema->GetSchemaID(),c_str,state,pSchema->NumMotions()));
   return pSchema->GetSchemaID();
#else
   return kGhostMotSchemaNoCustom;
#endif
}

#ifdef SHOCK
static void ShockFireListenFunc(ObjID obj, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   sGhostRemote *pGR = GhostGetRemote(obj);

   if (pGR == NULL)
      return;

   ObjID gun_archetype = GhostGetWeaponArch(pGR->weap.nWeapon);
   ObjID fake_gun = pGR->weap.weaponObj;
   ObjID projID = GetProjectile(gun_archetype);

   if ((gun_archetype == NULL) || (fake_gun == NULL) || (projID == NULL))
      return;

   // Check what type of weapon we're holding, and create the
   // appropriate sound.  Also create the appropriate muzzle flash
   // at the correct location.

   CreateGunFlashes(gun_archetype, fake_gun, 0, TRUE);

   if (projID != NULL)
   {
      // play firing sound
      cTagSet tagSet("Event Shoot");
      // @TODO: make this the real weapon mode
      tagSet.Add(cTag("WeaponMode", int(0)));
      // Don't network the sound for all players
      sSchemaCallParams params = g_sDefaultSchemaCallParams;
      params.flags |= SCH_NO_NETWORK;
      ObjPos *pHandPos = ObjPosGet(fake_gun);
      ESndPlayLoc(&tagSet, gun_archetype, projID, &(pHandPos->loc.vec), &params);
   }
}
#endif

void ShockInstallFireListenFunc(ObjID obj)
{
#ifdef SHOCK
   AddMotionFlagListener(obj, MF_FIRE_RELEASE, ShockFireListenFunc);
#endif
}

// shock specific ghost initialization
// basically, just stuffing some callbacks
// maybe someday setting detail/prefs/type or something
void ShockGhostInit(void)
{
   GhostPlayerActionCallback=ShockPlayerActionToSchemaIdx;
   GhostWeaponObjSwizzle=ShockSwizzleWeaponObj;
   GhostWeaponObjAttach=ShockDeSwizzleWeaponObj;
   GhostInstallFireListenFunc=ShockInstallFireListenFunc;
}

