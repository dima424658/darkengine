/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/basemode.c,v 1.32 2000/02/19 12:29:37 toml Exp $

#include <stdio.h>

#include <lg.h>
#include <loopapi.h>
#include <appagg.h>

#include <config.h>
#include <cfgdbg.h>

#include <netloop.h>
#include <basemode.h>
#include <testloop.h>
#include <ailoop.h>
#include <editgeom.h>
#include <brloop.h>
#include <objloop.h>
#include <rendloop.h>
#include <physloop.h>
#include <plyrloop.h>
#include <wrloop.h>
#include <schloop.h>
#include <simtloop.h>
#include <sndloop.h>
#include <arloop.h>
#include <roomloop.h>
#include <quesloop.h>
#include <biploop.h>
#include <antxloop.h>
#include <fvarloop.h>
#include <crwpnlup.h>

#include <editor.h>
#include <scrndump.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////
// LOOPMODE DESCRIPTOR
//

//
// Here is the list of the loop clients that this mode uses.
// Most clients are implemented in fooloop.c
//

#define GAMESPEC_RESERVED_CLIENT 0 

static tLoopClientID* _Clients[] =
{
   &GUID_NULL,  // reserved for gamespec
   &LOOPID_Test,
   &LOOPID_Base,
   &LOOPID_ObjSys,
   &LOOPID_AI,
   &LOOPID_Player,
   &LOOPID_PhysicsBase,
   &LOOPID_Wr,
   &LOOPID_Schema,
   &LOOPID_SimTime,
   &LOOPID_Sound,
   &LOOPID_ActReact,
   &LOOPID_Room,
   &LOOPID_Biped,
   &LOOPID_RenderBase, 
   &LOOPID_Quest,
   &LOOPID_AnimTexture,
   &LOOPID_FileVar, 
   &LOOPID_CreatureWeapon,

#ifdef EDITOR
   &LOOPID_EditGeom,
   &LOOPID_BrushList,
#endif // EDITOR

#ifdef NEW_NETWORK_ENABLED
   &LOOPID_Network,
   &LOOPID_NetworkReset,
   &LOOPID_ObjNet,
   &LOOPID_Ghost,
#endif
};

//
// Here's the actual loopmode descriptor
// It gets added to the loop manager in loopapp.c
//

sLoopModeDesc BaseLoopMode =
{
   { &LOOPID_BaseMode, "Base Mode"}, 
   _Clients,
   sizeof(_Clients)/sizeof(_Clients[0]),
};


////////////////////////////////////////

void BaseModeSetGameSpecClient(const GUID* clientID)
{
   _Clients[GAMESPEC_RESERVED_CLIENT] = clientID;
}


/////////////////////////////////////////////////////////////
// BASE LOOP CLIENT
////////////////////////////////////////////////////////////

// this is a generic loop client for simple lightweight stuff that 
// needs to be in the base 
// mode and doesn't quite merit its own loop client.

// This client has no context data.  If it did, this type would not be void.
// Your client could have any type it wants.  
typedef void Context;

// The client stores all its state here, plus a pointer to its context. 
// This client happens to have no state.

typedef struct _StateRecord
{
   Context* context;
} StateRecord;

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.


#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   sLoopFrameInfo* info = (msg & kMsgsFrame) ? (sLoopFrameInfo*)hdata : NULL;


   switch(msg)
   {
      case kMsgEnterMode:
         ConfigSpew("base_mode",("Base mode enter\n"));
         break;
      case kMsgEndFrame:
         DumpScreenMaybe(); 
         break;
      case kMsgEnd:
         Free(state);
         break;
   }
   return result;
}

// 
// Loop client factory function. 
//

#pragma off(unreferenced)

static ILoopClient* LGAPI _CreateClient(const sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord *)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,&BaseLoopClientDesc);
}
#pragma on(unreferenced)

//
// The loop client descriptor
// 

sLoopClientDesc BaseLoopClientDesc =
{
   &LOOPID_Base,               // client's guid
   "Base Client",              // string name
   kPriorityNormal,              // priority
   kMsgsMode | kMsgsFrame|kMsgEnd, // messages we want

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   { kNullConstraint } // terminator
};


