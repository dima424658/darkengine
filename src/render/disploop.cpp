/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/disploop.cpp,v 1.12 1998/10/05 17:22:57 mahk Exp $

#include <lg.h>
#include <loopapi.h>

#include <dispbase.h>  
#include <loopmsg.h>

#include <dispapi.h>
#include <disploop.h>

#include <config.h>

#include <scrnman.h>
#include <scrnguid.h>

extern "C"
{
#include <lgd3d.h>
#include <g2.h>
}

// must be last header
#include <dbmem.h>


/////////////////////////////////////////////////////////////
// DISPLAY LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


#define MY_FACTORY DisplayLoopFactory

#define MY_GUID   LOOPID_Display // My context data
typedef sDisplayLoopContext Context;

// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.

   struct sOldState
   {
      BOOL d3d; 
      int kind;
      int flags;
      GUID* DD; 
   }  old; 
} StateRecord;

////////////////////////////////////////
// 
// Perform a kind transition
//
EXTERN BOOL d3d_mode;
EXTERN BOOL g_lgd3d;
EXTERN int g_lgd3d_device_index;

static void compute_kind(StateRecord* state)
{
   int kind = state->context->kind; 
   ulong loopflags = state->context->flags; 
   
   int flags = 0;
   GUID *pDDrawGuid = NULL; 
   g_lgd3d = FALSE; 

   if (loopflags & kDispLoopPreserveDDraw)
      pDDrawGuid = (GUID*)&SCRNMAN_PRESERVE_DDRAW;

   if (d3d_mode && !(loopflags & kDispLoopProhibitD3D)) 
   {
      if (kind == kDispFullScreen)
      {  

         int num_devices = lgd3d_enumerate_devices();
         int idx = num_devices - 1; 
         if (num_devices <= 0)
            Error(1, "No direct 3d devices available.");

         if (config_get_int("d3d_driver_index",&idx))
         {
            if (idx < 0) idx += num_devices; 
            if (idx < 0 || idx >= num_devices)
            {
               idx = num_devices -1; 
               Warning(("D3D driver index %d is out of range, using %d",idx)); 
            }
         }

         flags = kDispAttemptFlippable|kDispAttempt3D;
         pDDrawGuid = lgd3d_get_device_info(idx)->p_ddraw_guid;

         g_lgd3d_device_index = idx;
         g_lgd3d = TRUE; 
      }
   } 


   state->old.kind = kind;
   state->old.flags = flags;
   state->old.DD = pDDrawGuid; 
   state->old.d3d = g_lgd3d; 
}

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
      case kMsgAppInit:
         d3d_mode = config_is_defined("use_d3d");
         break;
      case kMsgEnterMode:
      case kMsgResumeMode:
         compute_kind(state); 
         ScrnSetDisplay(state->old.kind,state->old.flags,state->old.DD); 
         break; 

      case kMsgEnd:
         Free(state);
         break;   
   }
   return result;
}

/////////////////
// CONSTRAINTS
//


static sRelativeLoopConstraint _Constraints[] = 
{

   {kNullConstraint} // terminator
};

///////////////
// DESCRIPTOR
// 



static sLoopClientDesc _LoopDesc =
{
   &MY_GUID,                           // GUID
   "Display Client",                    // NAME        
   kPriorityNormal,                    // PRIORITY          
   kMsgEnd | kMsgsMode | kMsgsAppOuter,   // INTERESTS      
   _Constraints,                       // CONSTRAINTS                
};

////////////////////////////////////////////////////////////
// Frome here on in is boiler plate code.
// Nothing need be changed.
//


// 
// Loop client factory function. 
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(tLoopClientData data, void *unused)
{
   sLoopClientDesc* desc = (sLoopClientDesc*)unused;
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

// 
// The simple factory descriptor
// This gets turned into a factory in loopapp.c
//

sLoopFactoryDesc MY_FACTORY = 
{ &MY_GUID, _CreateClient, &_LoopDesc}; 






