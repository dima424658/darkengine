/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover engine specific script stuff
//
#pragma once

#ifndef __DPCSCRPT_H
#define __DPCSCRPT_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

#include <scrptmsg.h>

#ifndef __ENGSCRPT_H
#include <engscrpt.h>
#endif // !__ENGSCRPT_H

#ifndef __DPCSCRM_H
#include <dpcscrm.h>
#endif // !__DPCSCRM_H

#ifndef __DAMGSCRP_H
#include <damgscrp.h>
#endif // !__DAMGSCRP_H

#ifndef __REPSCRPT_H
#include <repscrpt.h>
#endif // !__REPSCRPT_H


///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Game system and interface support functions
//

DECLARE_SCRIPT_SERVICE(DPCGame, 0x108)
{
   STDMETHOD(DestroyCursorObj)() PURE;
   STDMETHOD(DestroyInvObj)(const object& DestroyObj) PURE;
   STDMETHOD(HideInvObj)(const object& DestroyObj) PURE;

   STDMETHOD(AttachCamera)(const string& s) PURE;
   STDMETHOD(CutSceneModeOn)(const string& sceneName) PURE;
   STDMETHOD(CutSceneModeOff)(void) PURE;
   STDMETHOD_(int, CreatePlayerPuppet)(void) PURE;
   STDMETHOD_(int, CreatePlayerPuppet)(const string& modelName) PURE;
   STDMETHOD(DestroyPlayerPuppet)(void) PURE;

   STDMETHOD(Container)(const object& ContainObj) PURE;
   STDMETHOD(YorN)(const object& BaseObj, const string& s) PURE;
   STDMETHOD(Keypad)(const object& BaseObj) PURE;
   STDMETHOD(HRM)(integer hacktype, const object& Obj, BOOL frompsi) PURE;
   STDMETHOD(TechTool)(const object& Obj) PURE;

   STDMETHOD(UseLog)(const object& LogObj, BOOL PickedUpByMe) PURE;
   // types: 0: email 1: log 2: media 3: video
   STDMETHOD_(BOOL, TriggerLog)(int usetype, int uselevel, int which, BOOL show_mfd) PURE;
   STDMETHOD(FindLogData)(const object& LogObj, int usetype, int *level, int *which) PURE;

   STDMETHOD(PayNanites)(int quan) PURE;
   STDMETHOD(OverlayChange)(int which, int mode) PURE;

   STDMETHOD_(object, Equipped)(int slot) PURE;

   STDMETHOD(LevelTransport)(const char *newlevel, int marker, uint flags) PURE;
   STDMETHOD_(BOOL, CheckLocked)(const object& CheckObj, BOOL verbose, const object& player) PURE;

   // In both of these and AddTextObjProp(), "player" can be OBJ_NULL, in
   // which case the text will be shown to all players.
   STDMETHOD(AddText)(const char *msg, const object& player, int time = 5000) PURE;
   STDMETHOD(AddTranslatableText)(const char *msg, const char *table, const object& player, int time = 5000) PURE;

   STDMETHOD(AmmoLoad)(const object& GunObj, const object& AmmoObj) PURE;
   STDMETHOD_(int,GetClip)(const object& GunObj) PURE;

//   STDMETHOD(AddExp)(const object& Who, int amount, BOOL verbose ) PURE;
   STDMETHOD_(BOOL,HasTrait)(const object& Who, int trait) PURE;
   STDMETHOD(HealObj)(const object& Who, int amt) PURE;

   STDMETHOD(OverlaySetObj)(int which, const object& Obj) PURE;

   STDMETHOD_(string,GetArchetypeName)(const object& Obj) PURE;
   STDMETHOD_(BOOL, OverlayOn)(int which) PURE;

   STDMETHOD_(object,FindSpawnPoint)(const object& Obj, uint flags) PURE;
   STDMETHOD_(int, CountEcoMatching)(int val) PURE;

   STDMETHOD_(int, GetStat)(const object& who, int which) PURE;

   STDMETHOD_(object, GetSelectedObj)(void) PURE;

   STDMETHOD_(BOOL, AddInvObj)(const object& obj) PURE;

   STDMETHOD(RecalcStats)(const object& who) PURE;

   STDMETHOD(PlayVideo)(const char *vidname) PURE;

   STDMETHOD_(void, SetPlayerVolume)(float volume) PURE;

   STDMETHOD_(int, RandRange)(int low, int high) PURE;

   // NOTE: don't call this unless you're sure that there isn't an object currently on the cursor
   STDMETHOD_(BOOL, LoadCursor)(const object& obj) PURE;

   // speed control functions
   STDMETHOD_(void,AddSpeedControl)(const char *name, float speed_fac, float rot_fac) PURE;
   STDMETHOD_(void,RemoveSpeedControl)(const char *name) PURE;

   // prevent an obj-obj interaction from swapping
   STDMETHOD(PreventSwap)(void) PURE;

   // @NOTE:  I don't think we need this - unless we put magnetic watch back on the tasklist.  - Bodisafa
   // get distant shock-selected object 
   STDMETHOD_(object, GetDistantSelectedObj)(void) PURE;

   STDMETHOD(Equip)(int slot, const object& Obj) PURE;

   STDMETHOD(OverlayChangeObj)(int which, int mode, const object& Obj) PURE;

   STDMETHOD(SetObjState)(const object& Obj, int state) PURE;

   STDMETHOD(DestroyAllByName)(const char *name) PURE;

   STDMETHOD(AddTextObjProp)(const object& Obj, const char *propname, const object& player, int time = 5000) PURE;

   STDMETHOD(DisableAlarmGlobal)(void) PURE;

   STDMETHOD_(void, Frob)(BOOL in_inv = FALSE) PURE;

   STDMETHOD(TweqAllByName)(const char *name, BOOL state) PURE;

   STDMETHOD(SetExplored)(int maploc, char val = 1) PURE;

   // This should be called by any script that potentially removes
   // an object from a container.
   STDMETHOD(RemoveFromContainer)(const object& Obj, const object& Container) PURE;

   STDMETHOD(ActivateMap)(void) PURE;

   STDMETHOD_(int, SimTime)(void) PURE;

   STDMETHOD_(void, StartFadeIn)(int time, uchar red, uchar green, uchar blue) PURE;
   STDMETHOD_(void, StartFadeOut)(int time, uchar red, uchar green, uchar blue) PURE;

   STDMETHOD(PlayerMode)(int mode) PURE;
   STDMETHOD(EndGame)(void) PURE;

   STDMETHOD_(BOOL, AllowDeath)(void) PURE;

   STDMETHOD(AddAlarm)(int time) PURE;
   STDMETHOD(RemoveAlarm)(void) PURE;

   STDMETHOD_(float,GetHazardResistance)(int endur) PURE;
   
   STDMETHOD_(object,PlayerGun)(void) PURE;
   
   STDMETHOD_(object, PseudoProjectile)(const object& source, const object& emittype) PURE;

   STDMETHOD(WearArmor)(const object& Obj) PURE;

   STDMETHOD(SetModify)(const object& Obj, int modlevel) PURE;

   STDMETHOD_(BOOL, Censored)(void) PURE;

   STDMETHOD(TlucTextAdd)(char *name, char *table, int offset) PURE;

   STDMETHOD(Mouse)(BOOL mode, BOOL clear) PURE;

   STDMETHOD(RefreshInv)(void) PURE;
   STDMETHOD(TreasureTable)(const object& Obj) PURE;

   STDMETHOD_(object, OverlayGetObj)() PURE;

   STDMETHOD(VaporizeInv)(void) PURE;

   STDMETHOD(SetQBHacked)(const string& qbname, int qbval) PURE;

   STDMETHOD(SetLogTime)(int level, int logtype, int which) PURE;

   STDMETHOD(AddTranslatableTextInt)(const char *msg, const char *table, const object& player, int val, int time = 5000) PURE;

   STDMETHOD(ZeroControls)(const object& Obj, BOOL poll) PURE;

   STDMETHOD_(BOOL, ValidGun)(const object& Obj) PURE;

   STDMETHOD(AddTranslatableTextIndexInt)(const char *msg, const char *table, const object& player, int index, int val, int time = 5000) PURE;

   STDMETHOD_(BOOL, IsAlarmActive)(void) PURE;

   STDMETHOD(SlayAllByName)(const char *name) PURE;

   STDMETHOD(NoMove)(BOOL move_disallowed) PURE;

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

#endif /* !__DPCSCRPT_H */
