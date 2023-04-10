/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// crosshairs in center of screen
#include <2d.h>

#include <resapi.h>
#include <scrnmode.h>

#include <dpccross.h>
#include <dpcovrly.h>
#include <dpcutils.h>

static IRes *gCrosshairHnd;
static Point gCrosshairCenter;

//--------------------------------------------------------------------------------------
void DPCCrosshairInit(int which)
{
    gCrosshairHnd= LoadPCX("crosshai");

    sScrnMode smode;
    ScrnModeGet(&smode);

    gCrosshairCenter.x = smode.w / 2;
    gCrosshairCenter.y = smode.h / 2;
}

//--------------------------------------------------------------------------------------
void DPCCrosshairTerm(void)
{
    SafeFreeHnd(&gCrosshairHnd);
}

//--------------------------------------------------------------------------------------
void DPCCrosshairDraw(unsigned long inDeltaTicks)
{
    // @TBD: can this be hardcoded? Or do we need to deal with screen
    // resolutions here?
    DrawCursorByHandleCenter(gCrosshairHnd, gCrosshairCenter);
}

//--------------------------------------------------------------------------------------
sOverlayFunc OverlayCrosshair = 
{ 
    DPCCrosshairDraw,         // draw
    DPCCrosshairInit,         // init
    DPCCrosshairTerm,         // term
    NULL,                     // mouse
    NULL,                     // dclick (really use)
    NULL,                     // dragdrop
    NULL,                     // key
    NULL,                     // bitmap
    "",                       // upschema
    "",                       // downschema
    NULL,                     // state
    DPCOverlayFullTransp,     // transparency
    0,                        // distance
    FALSE,                    // needmouse
    85,                       // alpha
};
