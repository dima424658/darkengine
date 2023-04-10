/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simloop.c,v 1.4 2000/02/19 13:27:46 toml Exp $

#include <lg.h>
#include <loopapi.h>

#include <dispbase.h>
#include <loopmsg.h>

#include <plyrloop.h>
#include <simtime.h>
#include <simloop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////////////////////////////////////////////////////////
// SIM LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


//
//
#define MY_GUID   LOOPID_SimFinish

//
//
// My context data
typedef void Context;

//
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
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata; 


   switch(msg)
   {
      case kMsgEnterMode:
         SetSimTimeScale(1.0);
         break;
      case kMsgExitMode:
         SetSimTimeScale(1.0);
         break;

      case kMsgNormalFrame:
         break;

      case kMsgSuspendMode:
         break;
      case kMsgResumeMode:
         break;
      case kMsgEnd:
         Free(state);
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



sLoopClientDesc SimLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Sim Loop Client",                    // NAME        
   // @INSTRUCTIONS: Change your priority as necessary (see prikind.h)
   kPriorityNormal,                    // PRIORITY          
   // @INSTRUCTIONS: Change the set of messages you want to receive (see looptype.h)
   kMsgEnd | kMsgsMode | kMsgsFrameMid,   // INTERESTS      

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_Player, kMsgsFrame},
      {kNullConstraint} // terminator
   }
};
