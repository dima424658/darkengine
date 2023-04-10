/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/plyrhdlr.cpp,v 1.12 1998/05/13 10:34:00 kate Exp $

#include <plyrhdlr.h>
#include <plycbllm.h>
#include <mprintf.h>

#include <dbmem.h> // must be last included header

// @TODO: make this cleaner by folding various callbacks into 
// PlayerLimbHandler class, and by merging
// PlayerLimbHandler and PlayerAbility classes.

typedef struct
{
   fPlayerFrameCallback callback;
   // @TODO: actually pass this around!
   void *data;
} sPlayerFrameCallback;

typedef struct
{
   fPlayerModeCallback callback;
   void *data;
} sPlayerModeCallback;   

typedef struct
{
   fPlayerAbilityCallback callback;
   void *data;
} sPlayerAbilityCallback;   

sPlayerFrameCallback  *g_aPlayerFrameCallbacks;
sPlayerModeCallback *g_aPlayerEnterModeCallbacks;
sPlayerModeCallback *g_aPlayerLeaveModeCallbacks;
sPlayerAbilityCallback *g_aPlayerAbilityCallbacks;
sPlayerModeCallback *g_aPlayerIdleModeCallbacks;

IPlayerLimbHandler **g_apPlayerLimbHandlers;

static int g_CurrentMode=kPlayerModeInvalid;
static int g_nPlayerModes=0;

// set callback to use for given service.
// if currently using that service, start using the callback right away
void PlayerHandlerSetFrameCallback(int mode, fPlayerFrameCallback callback, void *data)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes), "Invalid player arm mode: %d\n", mode);

   // Hey kate, is this right? - MAHK
   if (mode == kPlayerModeInvalid)
      return; 

   g_aPlayerFrameCallbacks[mode].callback = callback;
   g_aPlayerFrameCallbacks[mode].data = data;
      

   if ((g_CurrentMode == mode) && (mode != kPlayerModeInvalid))
      PlayerCbllmSetFrameCallback(g_aPlayerFrameCallbacks[mode].callback);
}

void PlayerHandlerSetEnterModeCallback(int mode, fPlayerModeCallback callback, void *data)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes), "Invalid player arm mode: %d\n", mode);

   if ((mode < g_nPlayerModes) && (mode >= 0))
   {
      g_aPlayerEnterModeCallbacks[mode].callback = callback;
      g_aPlayerEnterModeCallbacks[mode].data = data;
   }
}

void PlayerHandlerSetLeaveModeCallback(int mode, fPlayerModeCallback callback, void *data)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes), "Invalid player arm mode: %d\n", mode);

   if ((mode < g_nPlayerModes) && (mode >= 0))
   {
      g_aPlayerLeaveModeCallbacks[mode].callback = callback;
      g_aPlayerLeaveModeCallbacks[mode].data = data;
   }
}

void PlayerHandlerSetAbilityCallback(int mode, fPlayerAbilityCallback callback, void *data)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes), "Invalid player arm mode: %d\n", mode);

   if ((mode < g_nPlayerModes) && (mode >= 0))
   {
      g_aPlayerAbilityCallbacks[mode].callback = callback;
      g_aPlayerAbilityCallbacks[mode].data = data;
   }
}

void PlayerHandlerSetIdleModeCallback(int mode, fPlayerModeCallback callback, void *data)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes), "Invalid player arm mode: %d\n", mode);

   if ((mode < g_nPlayerModes) && (mode >= 0))
   {
      g_aPlayerIdleModeCallbacks[mode].callback = callback;
      g_aPlayerIdleModeCallbacks[mode].data = data;
   }
}

static void PlayerHandlerEnterMode(int mode)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes) || (mode == kPlayerModeInvalid),
              "Invalid player arm mode: %d\n", mode);

   g_CurrentMode=mode;

   // Hey kate, is this right? - MAHK
   if (mode == kPlayerModeInvalid)
      return; 

   PlayerCbllmSetFrameCallback(g_aPlayerFrameCallbacks[mode].callback);

   if (g_aPlayerEnterModeCallbacks[mode].callback != NULL)
      g_aPlayerEnterModeCallbacks[mode].callback(mode, g_aPlayerEnterModeCallbacks[mode].data);
}

static void PlayerHandlerAbility(int mode, cPlayerAbility **pAbility, void *data)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes) || (mode == kPlayerModeInvalid),
              "Invalid player arm mode: %d\n", mode);

   // Hey kate, is this right? - MAHK
   if (mode == kPlayerModeInvalid)
      return; 

   if (g_aPlayerAbilityCallbacks[mode].callback != NULL)
      g_aPlayerAbilityCallbacks[mode].callback(mode, pAbility, g_aPlayerAbilityCallbacks[mode].data);
   else
      *pAbility=NULL;
}

static void PlayerHandlerLeaveMode(int mode)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes) || (mode == kPlayerModeInvalid),
              "Invalid player arm mode: %d\n", mode);

   g_CurrentMode=kPlayerModeInvalid;

   PlayerCbllmSetFrameCallback(NULL);

   // Hey kate, is this right? - MAHK
   if (mode == kPlayerModeInvalid)
      return; 

   if (g_aPlayerLeaveModeCallbacks[mode].callback != NULL)
      g_aPlayerLeaveModeCallbacks[mode].callback(mode, g_aPlayerLeaveModeCallbacks[mode].data);
}

void PlayerHandlerIdleMode(int mode)
{
   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes) || (mode == kPlayerModeInvalid),
              "Invalid player arm mode: %d\n", mode);

   // Hey kate, is this right? - MAHK
   if (mode == kPlayerModeInvalid)
      return; 

   if (g_aPlayerIdleModeCallbacks[mode].callback != NULL)
      g_aPlayerIdleModeCallbacks[mode].callback(mode, g_aPlayerIdleModeCallbacks[mode].data);
}

BOOL PlayerHandlerFrame(ulong dt, void *data)
{
   int mode=(int)data;

   AssertMsg1((mode >= 0) && (mode < g_nPlayerModes) || (mode == kPlayerModeInvalid),
              "Invalid player arm mode: %d\n", mode);

   // Hey kate, is this right? - MAHK
   if (mode == kPlayerModeInvalid)
      return FALSE; 

   if (g_aPlayerFrameCallbacks[mode].callback != NULL)
      g_aPlayerFrameCallbacks[mode].callback(dt, g_aPlayerFrameCallbacks[mode].data);

   return FALSE;
}

void PlayerHandlersTerm()
{
   g_nPlayerModes=0;

   if(g_aPlayerFrameCallbacks)
   {
      delete g_aPlayerFrameCallbacks;
      g_aPlayerFrameCallbacks=NULL;
   }
   if(g_aPlayerEnterModeCallbacks)
   {
      delete g_aPlayerEnterModeCallbacks;
      g_aPlayerEnterModeCallbacks=NULL;
   }
   if(g_aPlayerLeaveModeCallbacks)
   {
      delete g_aPlayerLeaveModeCallbacks;
      g_aPlayerLeaveModeCallbacks=NULL;
   }
   if(g_aPlayerAbilityCallbacks)
   {
      delete g_aPlayerAbilityCallbacks;
      g_aPlayerAbilityCallbacks=NULL;
   }
   if(g_aPlayerIdleModeCallbacks)
   {
      delete g_aPlayerIdleModeCallbacks;
      g_aPlayerIdleModeCallbacks=NULL;
   }
   if(g_apPlayerLimbHandlers)
   {
      delete g_apPlayerLimbHandlers;
      g_apPlayerLimbHandlers=NULL;
   }
}

void PlayerHandlerRegister(int mode, IPlayerLimbHandler *pHandler)
{
   Assert_(mode>=0&&mode<g_nPlayerModes);

   g_apPlayerLimbHandlers[mode]=pHandler;
}

void PlayerHandlersInit(int nPlayerModes)
{
   int i;

   // make sure was shut down first
   PlayerHandlersTerm();

   PlayerCbllmSetEnterModeCallback(PlayerHandlerEnterMode);
   PlayerCbllmSetLeaveModeCallback(PlayerHandlerLeaveMode);
   PlayerCbllmSetAbilityCallback(PlayerHandlerAbility);
   PlayerCbllmSetIdleCallback(PlayerHandlerIdleMode);
   PlayerCbllmSetFrameCallback(PlayerHandlerFrame);

   g_aPlayerFrameCallbacks=new sPlayerFrameCallback[nPlayerModes];
   g_aPlayerEnterModeCallbacks=new sPlayerModeCallback[nPlayerModes];
   g_aPlayerLeaveModeCallbacks=new sPlayerModeCallback[nPlayerModes];
   g_aPlayerAbilityCallbacks=new sPlayerAbilityCallback[nPlayerModes];
   g_aPlayerIdleModeCallbacks=new sPlayerModeCallback[nPlayerModes];
   g_apPlayerLimbHandlers=new IPlayerLimbHandler *[nPlayerModes];

   for (i=0; i<nPlayerModes; i++)
   {
      g_aPlayerFrameCallbacks[i].callback = NULL;
      g_aPlayerFrameCallbacks[i].data = NULL;

      g_aPlayerEnterModeCallbacks[i].callback = NULL;
      g_aPlayerEnterModeCallbacks[i].data = NULL;

      g_aPlayerLeaveModeCallbacks[i].callback = NULL;
      g_aPlayerLeaveModeCallbacks[i].data = NULL;

      g_aPlayerAbilityCallbacks[i].callback = NULL;
      g_aPlayerAbilityCallbacks[i].data = NULL;

      g_aPlayerIdleModeCallbacks[i].callback = NULL;
      g_aPlayerIdleModeCallbacks[i].data = NULL;

      g_apPlayerLimbHandlers[i]=NULL;
   }
   g_nPlayerModes=nPlayerModes;
}

/////////////////////////////////////////
//
// Script service hooks
//

BOOL PlayerHandlerStartUse(int mode)
{
   Assert_(mode>=0&&mode<g_nPlayerModes);

   if(!g_apPlayerLimbHandlers[mode])
      return FALSE;
   g_apPlayerLimbHandlers[mode]->StartUse();
   return TRUE;
}

BOOL PlayerHandlerFinishUse(int mode)
{
   Assert_(mode>=0&&mode<g_nPlayerModes);

   if(!g_apPlayerLimbHandlers[mode])
      return FALSE;
   g_apPlayerLimbHandlers[mode]->FinishUse();
   return TRUE;
}

BOOL PlayerHandlerEquip(int mode, ObjID item)
{
   Assert_(mode>=0&&mode<g_nPlayerModes);

   if(!g_apPlayerLimbHandlers[mode])
      return FALSE;
   return g_apPlayerLimbHandlers[mode]->Equip(item);
}

BOOL PlayerHandlerUnEquip(int mode, ObjID item)
{
   Assert_(mode>=0&&mode<g_nPlayerModes);

   if(!g_apPlayerLimbHandlers[mode])
      return FALSE;
   g_apPlayerLimbHandlers[mode]->UnEquip(item);
   return TRUE;
}
