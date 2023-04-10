/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// version indicator
#include <2d.h>

#include <resapi.h>
#include <playrobj.h>
#include <scrnmode.h>

#include <dpcver.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>

IRes *gVerHnd;

static Rect full_rect = {{640-128,0}, {640,128}};

//--------------------------------------------------------------------------------------
void DPCVersionInit(int which)
{
   gVerHnd = LoadPCX("alpha");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);

   short w = RectWidth(&full_rect);
   short h = RectHeight(&full_rect);
   use_rect.ul.x = smode.w - w;
   use_rect.ul.y = 0;
   use_rect.lr.x = use_rect.ul.x + w;
   use_rect.lr.y = use_rect.ul.y + h;

   DPCOverlaySetFlags(which, kOverlayFlagTranslucent);
   DPCOverlaySetRect(which, use_rect);
}

//--------------------------------------------------------------------------------------
void DPCVersionTerm(void)
{
   SafeFreeHnd(&gVerHnd);
}

//--------------------------------------------------------------------------------------
IRes *DPCVersionBitmap(void)
{
   return(gVerHnd);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayVersion = 
{ 
   NULL,                // draw
   DPCVersionInit,      // init
   DPCVersionTerm,      // term
   NULL,                // mouse
   NULL,                // dclick (really use)
   NULL,                // dragdrop
   NULL,                // key
   DPCVersionBitmap,    // bitmap
   "",                  // upschema
   "",                  // downschema
   NULL,                // state
   NULL,                // transparency
   0,                   // distance
   FALSE,               // needmouse
   30,                  // alpha
};
