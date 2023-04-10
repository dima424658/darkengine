/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkscrpt.h,v 1.102 2000/01/31 09:59:10 adurant Exp $
//
// Shock engine specific script stuff
//
#pragma once

#ifndef __SHKSCRPT_H
#define __SHKSCRPT_H

#include <objtype.h>

#include <scrptmsg.h>

#include <engscrpt.h>
#include <shkscrm.h>
#include <damgscrp.h>
#include <repscrpt.h>

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Game system and interface support functions
//

DECLARE_SCRIPT_SERVICE(ShockGame, 0x108)
{
   STDMETHOD(DestroyCursorObj)() PURE;
   STDMETHOD(DestroyInvObj)(const object ref DestroyObj) PURE;
   STDMETHOD(HideInvObj)(const object ref DestroyObj) PURE;

   STDMETHOD(SetPlayerPsiPoints)(int power) PURE;
   STDMETHOD_(int, GetPlayerPsiPoints)(void) PURE;

   STDMETHOD(AttachCamera)(const string ref s) PURE;
   STDMETHOD(CutSceneModeOn)(const string ref sceneName) PURE;
   STDMETHOD(CutSceneModeOff)(void) PURE;
   STDMETHOD_(int, CreatePlayerPuppet)(void) PURE;
   STDMETHOD_(int, CreatePlayerPuppet)(const string ref modelName) PURE;
   STDMETHOD(DestroyPlayerPuppet)(void) PURE;

   STDMETHOD(Replicator)(const object ref RepObj) PURE;
   STDMETHOD(Container)(const object ref ContainObj) PURE;
   STDMETHOD(YorN)(const object ref BaseObj, const string ref s) PURE;
   STDMETHOD(Keypad)(const object ref BaseObj) PURE;
   STDMETHOD(HRM)(integer hacktype, const object ref Obj, BOOL frompsi) PURE;
   STDMETHOD(TechTool)(const object ref Obj) PURE;

   STDMETHOD(UseLog)(const object ref LogObj, BOOL PickedUpByMe) PURE;
   // types: 0: email 1: log 2: media 3: video
   STDMETHOD_(BOOL, TriggerLog)(int usetype, int uselevel, int which, BOOL show_mfd) PURE;
   STDMETHOD(FindLogData)(const object ref LogObj, int usetype, int *level, int *which) PURE;

   STDMETHOD(PayNanites)(int quan) PURE;
   STDMETHOD(OverlayChange)(int which, int mode) PURE;

   STDMETHOD_(object, Equipped)(int slot) PURE;

   STDMETHOD(LevelTransport)(const char *newlevel, int marker, uint flags) PURE;
   STDMETHOD_(BOOL, CheckLocked)(const object ref CheckObj, BOOL verbose, const object ref player) PURE;

   // In both of these and AddTextObjProp(), "player" can be OBJ_NULL, in
   // which case the text will be shown to all players.
   STDMETHOD(AddText)(const char *msg, const object ref player, int time = 5000) PURE;
   STDMETHOD(AddTranslatableText)(const char *msg, const char *table, const object ref player, int time = 5000) PURE;

   STDMETHOD(AmmoLoad)(const object ref GunObj, const object ref AmmoObj) PURE;
   STDMETHOD_(int,GetClip)(const object ref GunObj) PURE;

   STDMETHOD(AddExp)(const object ref Who, int amount, BOOL verbose ) PURE;
   STDMETHOD_(BOOL,HasTrait)(const object ref Who, int trait) PURE;
   STDMETHOD_(BOOL,HasImplant)(const object ref Who, int implant) PURE;
   STDMETHOD(HealObj)(const object ref Who, int amt) PURE;

   STDMETHOD(OverlaySetObj)(int which, const object ref Obj) PURE;

   STDMETHOD(Research)(void) PURE;
   STDMETHOD_(string,GetArchetypeName)(const object ref Obj) PURE;
   STDMETHOD_(BOOL, OverlayOn)(int which) PURE;

   STDMETHOD_(object,FindSpawnPoint)(const object ref Obj, uint flags) PURE;
   STDMETHOD_(int, CountEcoMatching)(int val) PURE;

   STDMETHOD_(int, GetStat)(const object ref who, int which) PURE;

   STDMETHOD_(object, GetSelectedObj)(void) PURE;

   STDMETHOD_(BOOL, AddInvObj)(const object ref obj) PURE;

   STDMETHOD(RecalcStats)(const object ref who) PURE;

   STDMETHOD(PlayVideo)(const char *vidname) PURE;

   STDMETHOD(ClearRadiation)(void) PURE;

   STDMETHOD_(void, SetPlayerVolume)(float volume) PURE;

   STDMETHOD_(int, RandRange)(int low, int high) PURE;

   // NOTE: don't call this unless you're sure that there isn't an object currently on the cursor
   STDMETHOD_(BOOL, LoadCursor)(const object ref obj) PURE;

   // speed control functions
   STDMETHOD_(void,AddSpeedControl)(const char *name, float speed_fac, float rot_fac) PURE;
   STDMETHOD_(void,RemoveSpeedControl)(const char *name) PURE;

   // prevent an obj-obj interaction from swapping
   STDMETHOD(PreventSwap)(void) PURE;

   // get distant shock-selected object 
   STDMETHOD_(object, GetDistantSelectedObj)(void) PURE;

   STDMETHOD(Equip)(int slot, const object ref Obj) PURE;

   STDMETHOD(OverlayChangeObj)(int which, int mode, const object ref Obj) PURE;

   STDMETHOD(SetObjState)(const object ref Obj, int state) PURE;

   STDMETHOD(RadiationHack)(void) PURE;
   STDMETHOD(DestroyAllByName)(const char *name) PURE;

   STDMETHOD(AddTextObjProp)(const object ref Obj, const char *propname, const object ref player, int time = 5000) PURE;

   STDMETHOD(DisableAlarmGlobal)(void) PURE;

   STDMETHOD_(void, Frob)(BOOL in_inv = FALSE) PURE;

   STDMETHOD(TweqAllByName)(const char *name, BOOL state) PURE;

   STDMETHOD(SetExplored)(int maploc, char val = 1) PURE;

   // This should be called by any script that potentially removes
   // an object from a container.
   STDMETHOD(RemoveFromContainer)(const object ref Obj, const object ref Container) PURE;

   STDMETHOD(ActivateMap)(void) PURE;

   STDMETHOD_(int, SimTime)(void) PURE;

   STDMETHOD_(void, StartFadeIn)(int time, uchar red, uchar green, uchar blue) PURE;
   STDMETHOD_(void, StartFadeOut)(int time, uchar red, uchar green, uchar blue) PURE;

   STDMETHOD(GrantPsiPower)(const object ref who, int which) PURE;

   STDMETHOD_(BOOL, ResearchConsume)(const object ref Obj) PURE;
   STDMETHOD(PlayerMode)(int mode) PURE;
   STDMETHOD(EndGame)(void) PURE;

   STDMETHOD_(BOOL, AllowDeath)(void) PURE;

   STDMETHOD(AddAlarm)(int time) PURE;
   STDMETHOD(RemoveAlarm)(void) PURE;

   STDMETHOD_(float,GetHazardResistance)(int endur) PURE;
   STDMETHOD_(int,GetBurnDmg)(void) PURE;

   STDMETHOD_(object,PlayerGun)(void) PURE;
   
   STDMETHOD_(BOOL, IsPsiActive)(int power) PURE;
   STDMETHOD(PsiRadarScan)(void) PURE;
   STDMETHOD_(object, PseudoProjectile)(const object ref source, const object ref emittype) PURE;

   STDMETHOD(WearArmor)(const object ref Obj) PURE;

   STDMETHOD(SetModify)(const object ref Obj, int modlevel) PURE;

   STDMETHOD_(BOOL, Censored)(void) PURE;

   STDMETHOD(DebriefMode)(int mission) PURE;
   STDMETHOD(TlucTextAdd)(char *name, char *table, int offset) PURE;

   STDMETHOD(Mouse)(BOOL mode, BOOL clear) PURE;

   STDMETHOD(RefreshInv)(void) PURE;
   STDMETHOD(TreasureTable)(const object ref Obj) PURE;

   STDMETHOD_(object, OverlayGetObj)() PURE;

   STDMETHOD(VaporizeInv)(void) PURE;

   STDMETHOD(ShutoffPsi)(void) PURE;

   STDMETHOD(SetQBHacked)(const string ref qbname, int qbval) PURE;

   STDMETHOD_(int, GetPlayerMaxPsiPoints)(void) PURE;

   STDMETHOD(SetLogTime)(int level, int logtype, int which) PURE;

   STDMETHOD(AddTranslatableTextInt)(const char *msg, const char *table, const object ref player, int val, int time = 5000) PURE;

   STDMETHOD(ZeroControls)(const object ref Obj, BOOL poll) PURE;

   STDMETHOD(SetSelectedPsiPower)(int which) PURE;

   STDMETHOD_(BOOL, ValidGun)(const object ref Obj) PURE;

   STDMETHOD(AddTranslatableTextIndexInt)(const char *msg, const char *table, const object ref player, int index, int val, int time = 5000) PURE;

   STDMETHOD_(BOOL, IsAlarmActive)(void) PURE;

   STDMETHOD(SlayAllByName)(const char *name) PURE;

   STDMETHOD(NoMove)(BOOL jump_allowed) PURE;

   STDMETHOD(PlayerModeSimple)(int mode) PURE;

   STDMETHOD(UpdateMovingTerrainVelocity)(const object objID, const object next_node, float speed) PURE;

   STDMETHOD_(BOOL, MouseCursor)() PURE;

   STDMETHOD_(BOOL, ConfigIsDefined)(const char *name) PURE;

   STDMETHOD_(BOOL, ConfigGetInt)(const char *name, int ref value) PURE;
};


///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//


#ifdef SCRIPT

///////////////////////////////////////
//
// SCRIPT: RootScript
//

BEGIN_SCRIPT(RootScript, EngineRootScript)

   MESSAGE_DEFS: 
  
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(YorNDone, sYorNMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(KeypadDone, sKeypadMsg)
      DAMAGE_MESSAGE_DEFAULTS()
      REPORT_MESSAGE_DEFAULTS()

   SCRIPT_BEGIN_MESSAGE_MAP()
      DAMAGE_MESSAGE_MAP_ENTRIES()
      REPORT_MESSAGE_MAP_ENTRIES()
      SCRIPT_MSG_MAP_ENTRY(YorNDone)
      SCRIPT_MSG_MAP_ENTRY(KeypadDone)
   SCRIPT_END_MESSAGE_MAP()
END_SCRIPT(RootScript)

#endif


///////////////////////////////////////////////////////////////////////////////

#endif /* !__SHKSCRPT_H */
