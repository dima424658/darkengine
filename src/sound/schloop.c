/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schloop.c,v 1.16 2000/02/19 12:37:12 toml Exp $

#include <lg.h>
#include <loopapi.h>

#include <dispbase.h>
#include <loopmsg.h>

#include <schloop.h>

#include <ailoop.h>
#include <objloop.h>
#include <osysbase.h>
#include <dbasemsg.h>
#include <schema.h>
#include <schdb.h>
#include <schsamps.h>
#include <spchglue.h>
#include <esndglue.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_GUID LOOPID_Schema

// My context data
typedef void Context;

// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
} StateRecord;


////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgEnterMode:
         break;
      case kMsgExitMode:
         break;

      case kMsgNormalFrame:
         // the schema frame is in sndloop for recording purposes
         break;

      case kMsgDatabase:
         SchemaDatabaseNotify(info.dispatch);
         SpeechDatabaseNotify(info.dispatch);
         ESndDatabaseNotify(info.dispatch);
         break;

      case kMsgSuspendMode:
         break;
      case kMsgResumeMode:
         break;
      case kMsgEnd:
         Free(state);
         break;

      case kMsgAppInit:
         SchemaInit();
         SpeechInit();
         SchemaSamplesInit();
         ESndInit();
         break;

      case kMsgAppTerm:
         SchemaSamplesShutdown();
         SchemaShutdown();
         SpeechTerm();
         ESndTerm();
         break;
   }
   return result;
}

////////////////////////////////////////////////////////////
//
// Loop client factory function.
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//

sLoopClientDesc SchemaLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Schema Client",                    // NAME
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgDatabase | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter,  // INTERESTS

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase},
      {kConstrainAfter, &LOOPID_AI, kMsgDatabase},
      {kNullConstraint} // terminator
   }
};

