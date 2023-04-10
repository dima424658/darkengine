/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dispatch.cpp,v 1.10 1998/04/24 14:12:40 TOML Exp $
#include <loopapi.h>
#include <appagg.h>

#include <loopapp.h> // for allclients
#include <dispatch.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Base Mode Dispatch Chain
//

static       ILoopDispatch*   baseChain = NULL;
static const tLoopModeID*     baseID = &GUID_NULL;

static DispatchKind  CurKind = 0;
static DispatchData* CurData = NULL;

static BOOL Started = FALSE;

//
// Context for base mode
//

static sLoopModeInitParm Context[] =
{
   { NULL, },
};


////////////////////////////////////////////////////////////
// Update our reference to the base dispatch
//

#define DISPATCH_MSGS  (kMsgsAll & ~(kMsgsFrame|kMsgsMode|kMsgsState))

static void UpdateBaseMode(void)
{
   AutoAppIPtr(LoopManager);
   cAutoIPtr<ILoopMode> pBase;

   pBase = pLoopManager->GetBaseMode();
   SafeRelease(pLoopManager);

   if (pBase == NULL)
   {
      baseID = &GUID_NULL;
      baseChain = NULL;
      return;
   }

   // if the base mode has not changed, we're all set.
   if (*pBase->GetName()->pID == *baseID)
      return;

   // release the old dispatch chain
   SafeRelease(baseChain);

   baseID = pBase->GetName()->pID;
   pBase->CreatePartialDispatch(Context,DISPATCH_MSGS,&baseChain);
}

////////////////////////////////////////////////////////////
// API FUNCTIONS
//

void DispatchInit(void)
{
   UpdateBaseMode();
   Started = TRUE;
}

////////////////////////////////////////

void DispatchShutdown(void)
{
   SafeRelease(baseChain);
   baseID = &GUID_NULL;
   Started = FALSE;
}

////////////////////////////////////////

void DispatchMsg(DispatchKind kind, DispatchData* data)
{
   if (!Started) return;

   ILoop* looper = AppGetObj(ILoop);
   ILoopDispatch* disp = looper->GetCurrentDispatch();
   if (disp == NULL)
   {

      UpdateBaseMode();
      disp = baseChain;
   }
   if (disp != NULL)
   {
      DispatchKind SaveKind = CurKind;
      DispatchData* SaveData = CurData;
      CurKind = kind;
      CurData = data;
      disp->SendMessage((eLoopMessage)kind,(tLoopMessageData)data,kDispatchForward);
      CurKind = SaveKind;
      CurData = SaveData;
   }
   SafeRelease(looper);
}

////////////////////////////////////////////////////////////

void DispatchMsgAllClients(DispatchKind kind, DispatchData* data, int flags)
{
   if (!Started) return;

   AutoAppIPtr(LoopManager);
   cAutoIPtr<ILoopMode> mode;
   mode = pLoopManager->GetMode(&LOOPID_AllClients);
   ILoopDispatch* disp;
   // build the chain for that mode
   mode->CreatePartialDispatch(Context,kind,&disp);
   DispatchKind savekind = CurKind;
   DispatchData* savedata = CurData;
   CurKind = kind;
   CurData = data;
   disp->SendMessage((eLoopMessage)kind,(tLoopMessageData)data,flags);
   CurKind = savekind;
   CurData = savedata;
   SafeRelease(disp);
   SafeRelease(pLoopManager);
}

////////////////////////////////////////////////////////////

BOOL DispatchInMsg(DispatchKind mask)
{
   return (mask & CurKind) != 0;
}


BOOL DispatchCurrentMsg(DispatchKind* kind, DispatchData** data)
{
   if (CurKind == 0)
      return FALSE;
   *kind = CurKind;
   *data = CurData;
   return TRUE;
}

////////////////////////////////////////////////////////////
// DISPATCH MESSAGE KINDS
//
// Here is a list of available loopmode messages, and their meanings
// as dispatch messages.
//
// If you want to grab a new dispatch message, use a blank one from this table.
//
//    kMsgApp1             kMsgVisual        // change in visual state or appearance
//    kMsgApp2             kMsgGameSys       // game system messages
//    kMsgApp3             kMsgObjSys        // object system messages
//    kMsgApp4
//    kMsgApp5
//    kMsgApp6
//    kMsgApp7
//    kMsgApp8
//    kMsgLoad             kMsgDatabase      // Gross database changes (save,load, reset, etc)
//    kMsgUserReserved1    kMsgAppInit       // one-time app init.
//    kMsgUserReserved2    kMsgAppTerm       // one-time app term.
//    kMsgUserReserved3
//

