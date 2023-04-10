/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmlook.cpp,v 1.3 2000/02/19 13:25:44 toml Exp $

#include <2d.h>

#include <resapi.h>

#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkifstr.h>

#include <scrnmode.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

static IRes *gButtBack;

static Rect full_rect = {{281,434},{281 + 78, 434 + 44}};
static Rect close_rect = {{23,3},{23 + 32, 3 + 40}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

//--------------------------------------------------------------------------------------
void ShockMouseModeInit(int )
{
   int i;

   close_handles[0] = LoadPCX("ML0"); 
   close_handles[1] = LoadPCX("ML1"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   gButtBack = LoadPCX("MICELOOK");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);

   if (smode.w > 640)
   {
      use_rect.ul.x = (smode.w - RectWidth(&full_rect)) / 2;
      use_rect.ul.y = smode.h - RectHeight(&full_rect);
      use_rect.lr.x = use_rect.ul.x + RectWidth(&full_rect);
      use_rect.lr.y = use_rect.ul.y + RectHeight(&full_rect);
      ShockOverlaySetRect(kOverlayMouseMode,use_rect);
   }
   else
      ShockOverlaySetRect(kOverlayMouseMode,full_rect);
}

//--------------------------------------------------------------------------------------
void ShockMouseModeTerm(void)
{
   SafeFreeHnd(&gButtBack);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
void ShockMouseModeDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMouseMode);

   DrawByHandle(gButtBack,r.ul);

   LGadDrawBox(VB(&close_button),NULL);

   Point mpt;
   mouse_get_xy(&mpt.x,&mpt.y);
   if (RectTestPt(&r,mpt))
      ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpMouseMode","misc");
}
//--------------------------------------------------------------------------------------
void DeferredMouseMode(void *)
{
   MouseMode(FALSE,TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferredMouseMode,NULL);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMouseMode);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockMouseModeStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      BuildInterfaceButtons();
   }
   else
   {
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------

sOverlayFunc OverlayMouseMode = { 
   ShockMouseModeDraw, // draw
   ShockMouseModeInit, // init
   ShockMouseModeTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   ShockMouseModeStateChange, // state
   NULL, // transparency
   0, // distance
   TRUE, // needmouse
};
