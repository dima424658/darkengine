/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkrloop.c,v 1.11 2000/02/19 12:27:25 toml Exp $

#include <lg.h>
#include <loopapi.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <appsfx.h>

#include <dispbase.h>  
#include <loopmsg.h>
#include <bugterm.h>

#include <simloop.h>
#include <simflags.h>
#include <simstate.h>
#include <rendobj.h>
#include <rendloop.h> 
#include <scrnloop.h>
#include <objloop.h>

#include <drkloop.h>
#include <drkgame.h>
#include <drkrend.h>
#include <mnumprop.h>
#include <drkinvui.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


/////////////////////////////////////////////////////////////
// DARK RENDERER LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


#define MY_FACTORY DarkRenderLoopFactory
#define MY_GUID   LOOPID_DarkRender

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
   // useful stuff for most clients
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
         if (SimStateCheckFlags(kSimRender))
         {
            dark_rend_update_frame();
            InvUIRender(info.frame->nTicks); 
         }
         break;

      case kMsgSuspendMode:
         break;
      case kMsgResumeMode:
         break;

      case kMsgAppInit:
         dark_init_object_rend();
         break;

      case kMsgAppTerm:
         dark_term_object_rend();
         rendobj_exit();
         break;   

      case kMsgEnd:
         Free(state);
         break;   
   }
   return result;
}

////////////////////////////////////////////////////////////
// Frome here on in is boiler plate code.
// Nothing need be changed.
//


// 
// Loop client factory function. 
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * desc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 

sLoopClientDesc DarkRenderClientDesc =
{
   &MY_GUID,                           // GUID
   "Dark Rendering",                    // NAME        
   kPriorityNormal,                    // PRIORITY          
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter,   // INTERESTS      
   
   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      {kConstrainAfter, &LOOPID_SimFinish, kMsgsFrame}, 
      {kConstrainAfter, &LOOPID_Render, kMsgsFrameMid|kMsgsAppOuter}, 
      {kConstrainAfter, &LOOPID_ObjSys, kMsgsAppOuter}, 
      {kNullConstraint} // terminator
   }
};







