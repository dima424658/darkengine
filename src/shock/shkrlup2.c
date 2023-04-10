/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrlup2.c,v 1.6 2000/02/19 13:26:08 toml Exp $

// disabling whole file on account of pointlessness
#if 0

#include <lg.h>
#include <loopapi.h>
#include <mprintf.h>
#include <lgd3d.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <appsfx.h>

#include <dispbase.h>  
#include <loopmsg.h>

#include <simloop.h>
#include <shkloop.h>
#include <rendloop.h> 
#include <scrnloop.h>

#include <playrobj.h>
#include <shkgame.h>
#include <shkrend.h>
#include <mnumprop.h>
#include <camera.h>

#include <shkovrly.h>
#include <shkrlup2.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern BOOL g_lgd3d;
grs_canvas shock_rend_canvas;
Rect shock_rend_rect = {{0,0},{640,480}};

/////////////////////////////////////////////////////////////
// RENDERING LOOP, PART 2 (since we have both pre-and post render stages)
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


#define MY_FACTORY ShockRender2LoopFactory
#define MY_GUID   LOOPID_ShockRender2

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

      case kMsgNormalFrame:
         {
            Camera *camptr;
            //static int lastw = -1;
            //mxs_real oldzoom;
            //mxs_real val;

            gr_init_sub_canvas(grd_canvas, &shock_rend_canvas, shock_rend_rect.ul.x, shock_rend_rect.ul.y, 
               RectWidth(&shock_rend_rect), RectHeight(&shock_rend_rect));
            gr_push_canvas(&shock_rend_canvas);
            lgd3d_set_offsets(shock_rend_rect.ul.x, shock_rend_rect.ul.y);
            // compute FOV, which is in an inverse relationship to width
            camptr = PlayerCamera();
            /*
            oldzoom = camptr->zoom;
            if (lastw == -1)
               lastw = RectWidth(&shock_rend_rect);
            camptr->zoom = camptr->zoom * (mxs_real)lastw / (mxs_real)RectWidth(&shock_rend_rect);
            lastw = RectWidth(&shock_rend_rect);
            mprintf("%d vs %d, setting zoom to %g (was %g)\n",lastw,RectWidth(&shock_rend_rect),
               camptr->zoom, oldzoom);
            */
            /*
            val = 640.0F/480.0F;
            //camptr->zoom = val / ((mxs_real)RectWidth(&shock_rend_rect)/(mxs_real)RectHeight(&shock_rend_rect));
            val = 640 / RectWidth(&shock_rend_rect);
            camptr->zoom = val * val;
            mprintf("setting zoom to %g\n",camptr->zoom);
            */
         }
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

sLoopClientDesc ShockRender2ClientDesc =
{
   &MY_GUID,                           // GUID
   "Shock Rendering 2",                    // NAME        
   kPriorityNormal,                    // PRIORITY          
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter,   // INTERESTS      
   
   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      {kConstrainBefore, &LOOPID_Render, kMsgsFrameMid|kMsgsAppOuter}, 
      {kNullConstraint} // terminator
   }
};

#endif