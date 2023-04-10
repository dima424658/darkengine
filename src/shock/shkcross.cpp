/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcross.cpp,v 1.6 2000/02/19 12:36:43 toml Exp $

// crosshairs in center of screen
#include <2d.h>

#include <resapi.h>
#include <scrnmode.h>

#include <shkcross.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static IRes *gCrosshairHnd;
static Point gCrosshairCenter;

//--------------------------------------------------------------------------------------
void ShockCrosshairInit(int which)
{
   gCrosshairHnd= LoadPCX("crosshai");

   sScrnMode smode;
   ScrnModeGet(&smode);

   gCrosshairCenter.x = smode.w / 2;
   gCrosshairCenter.y = smode.h / 2;
}

//--------------------------------------------------------------------------------------
void ShockCrosshairTerm(void)
{
   SafeFreeHnd(&gCrosshairHnd);
}

//--------------------------------------------------------------------------------------
void ShockCrosshairDraw(void)
{
   // @TBD: can this be hardcoded? Or do we need to deal with screen
   // resolutions here?
   DrawCursorByHandleCenter(gCrosshairHnd, gCrosshairCenter);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayCrosshair = { 
   ShockCrosshairDraw, // draw
   ShockCrosshairInit, // init
   ShockCrosshairTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   NULL, // state
   ShockOverlayFullTransp, // transparency
   0, // distance
   FALSE, // needmouse
   85, // alpha
};
