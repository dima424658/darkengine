/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkver.cpp,v 1.5 2000/02/19 13:26:34 toml Exp $

// version indicator
#include <2d.h>

#include <resapi.h>
#include <playrobj.h>
#include <scrnmode.h>

#include <shkver.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gVerHnd;

static Rect full_rect = {{640-128,0},{640,128}};

//--------------------------------------------------------------------------------------
void ShockVersionInit(int which)
{
   gVerHnd = LoadPCX("alpha");

   short w,h;
   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);

   w = RectWidth(&full_rect);
   h = RectHeight(&full_rect);
   use_rect.ul.x = smode.w - w;
   use_rect.ul.y = 0;
   use_rect.lr.x = use_rect.ul.x + w;
   use_rect.lr.y = use_rect.ul.y + h;

   ShockOverlaySetFlags(which, kOverlayFlagTranslucent);
   ShockOverlaySetRect(which, use_rect);
}

//--------------------------------------------------------------------------------------
void ShockVersionTerm(void)
{
   SafeFreeHnd(&gVerHnd);
}

//--------------------------------------------------------------------------------------
IRes *ShockVersionBitmap(void)
{
   return(gVerHnd);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayVersion = { 
   NULL, // draw
   ShockVersionInit, // init
   ShockVersionTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockVersionBitmap, // bitmap
   "", // upschema
   "", // downschema
   NULL, // state
   NULL, // transparency
   0, // distance
   FALSE, // needmouse
   30, // alpha
};
