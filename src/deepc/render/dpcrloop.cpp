/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/deepc/render/dpcrloop.cpp,v 1.3 1999/12/15 17:12:31 MAT Exp $

/////////////////////////////////////////////////////////////
// Deep Cover simulation loop client
////////////////////////////////////////////////////////////

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
#include <dpcloop.h>
#include <rendloop.h>
#include <objloop.h> 
#include <scrnloop.h>
#include <scrnman.h>
#include <uiloop.h>

#include <dpcutils.h>
#include <objtxtrp.h>
#include <dpcgame.h>
#include <dpcrend.h>
#include <mnumprop.h>

#include <dpcovrly.h>

// from namedres
extern BOOL gResImageConvert8888to4444;

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


#define MY_FACTORY DPCRenderLoopFactory
#define MY_GUID LOOPID_DPCRender

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

    switch (msg)
    {
        case kMsgResumeMode:
        case kMsgEnterMode:
        {
            DPCOverlayInit();
            state->initted = TRUE;
            break;
        }
        case kMsgSuspendMode:
        case kMsgExitMode:
        {
            if (state->initted)
                DPCOverlayTerm();
            state->initted = FALSE; 
            break;
        }

        case kMsgNormalFrame:
        {
            if (SimStateCheckFlags(kSimRender))
            {
                DPCOverlayDrawBuffers();

                ScrnLockDrawCanvas();

                DPCOverlayDoFrame(info.frame->dTicks);
                if (DPC_mouse)
                {
                    extern void DPCDrawCursor(Point pt);
                    Point pt;
                    mouse_get_xy(&pt.x,&pt.y);
                    DPCDrawCursor(pt);
                    //DrawByHandleCenter(gCursorHnd,pt);
                }

                //air_hud();

                ScrnUnlockDrawCanvas();

                // DPCOverlayDrawBuffers();

                //update_player_medium_sounds();
                //DPC_rend_update_frame();
            }
            break;
        }

        case kMsgAppInit:
        {
            DPC_init_object_rend();
            DPC_init_renderer();
            gResImageConvert8888to4444 = FALSE;
            break;
        }
        case kMsgAppTerm:
        {
            DPC_term_object_rend();
            DPC_term_renderer();
            break;   
        }

        case kMsgEnd:
        {
            Free(state);
            break;   
        }
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

sLoopClientDesc DPCRenderClientDesc =
{
    &MY_GUID,                                                            // GUID
    "DPC Rendering",                                                     // NAME        
    kPriorityNormal,                                                     // PRIORITY          
    kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter,                 // INTERESTS      

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
        {kNullConstraint}                                                 // terminator
    }
};
