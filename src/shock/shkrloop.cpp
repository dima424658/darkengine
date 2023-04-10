/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrloop.cpp,v 1.26 2000/02/19 13:26:07 toml Exp $

#include <lg.h>
#include <loopapi.h>
#include <mprintf.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <appsfx.h>

#include <dispbase.h>  
#include <loopmsg.h>
#include <bugterm.h>

#include <simflags.h>
#include <simstate.h>
#include <simloop.h>
#include <shkloop.h>
#include <rendloop.h>
#include <objloop.h> 
#include <scrnloop.h>
#include <scrnman.h>
#include <uiloop.h>

#include <shkutils.h>
#include <shkgame.h>
#include <shkrend.h>
#include <mnumprop.h>

#include <shkovrly.h>
#include <shkrlup2.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern BOOL g_lgd3d;

/////////////////////////////////////////////////////////////
// AIR SIMULATION LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


#define MY_FACTORY ShockRenderLoopFactory
#define MY_GUID   LOOPID_ShockRender

// My context data
typedef void Context;

// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   BOOL initted; 
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
      case kMsgResumeMode:
      case kMsgEnterMode:
         ShockOverlayInit();
         state->initted = TRUE; 
         break;
      case kMsgSuspendMode:
      case kMsgExitMode:
         if (state->initted)
            ShockOverlayTerm();
         state->initted = FALSE; 
         break;
         break;

      case kMsgNormalFrame:
         //gr_pop_canvas(); // from the sub-canvas of before in shkrlup2
         //gr_close_canvas(&shock_rend_canvas);
         // a brutal, and godawful hack!
         // someone kill me now.

         if (SimStateCheckFlags(kSimRender))
         {
            ShockOverlayDrawBuffers();

            ScrnLockDrawCanvas();

            ShockOverlayDoFrame();
            if (shock_mouse) 
            {
               extern void ShockDrawCursor(Point pt);
               Point pt;
               mouse_get_xy(&pt.x,&pt.y);
               ShockDrawCursor(pt);
               //DrawByHandleCenter(gCursorHnd,pt);
            }

            //air_hud();

            ScrnUnlockDrawCanvas();

            // ShockOverlayDrawBuffers();

            //update_player_medium_sounds();
            //shock_rend_update_frame();
         }
         break;

      case kMsgAppInit:
         shock_init_object_rend();
         shock_init_renderer();
         break;
      case kMsgAppTerm:
         shock_term_object_rend();
         shock_term_renderer();
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
   memset(state,0,sizeof(*state)); 
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 

sLoopClientDesc ShockRenderClientDesc =
{
   &MY_GUID,                           // GUID
   "Shock Rendering",                    // NAME        
   kPriorityNormal,                    // PRIORITY          
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter,   // INTERESTS      
   
   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      {kConstrainAfter, &LOOPID_SimFinish, kMsgsFrame}, 
      {kConstrainAfter, &LOOPID_Render, kMsgsFrameMid|kMsgsAppOuter}, 
      {kConstrainAfter, &LOOPID_RenderBase, kMsgsAppOuter}, 
      {kConstrainAfter, &LOOPID_ObjSys, kMsgsAppOuter}, 
      {kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode}, 
      {kConstrainAfter, &LOOPID_UI, kMsgsMode},
      {kNullConstraint} // terminator
   }
};
