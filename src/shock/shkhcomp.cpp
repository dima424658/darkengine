/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhcomp.cpp,v 1.7 2000/02/19 13:25:21 toml Exp $

// alarm icon
#include <2d.h>

#include <resapi.h>
#include <appagg.h>

#include <playrobj.h>

#include <shkhcomp.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkhplug.h>
#include <shkobjst.h>
#include <shkprop.h>
#include <shkiftul.h>
#include <shkplayr.h>
#include <shktrcst.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static IRes *gComputerHnd;
static IRes *gBrokenHnd;

//--------------------------------------------------------------------------------------
void ShockComputerInit(int which)
{
   int i;
   gComputerHnd= LoadPCX("access");
   gBrokenHnd= LoadPCX("baccess");

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockComputerTerm(void)
{
   SafeFreeHnd(&gComputerHnd);
   SafeFreeHnd(&gBrokenHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
void ShockComputerDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHackComp);
   eObjState st;
   ObjID o = ShockOverlayGetObj();
   st = ObjGetObjState(o);

   if (st == kObjStateBroken)
      DrawByHandle(gBrokenHnd,r.ul);
   else
      DrawByHandle(gComputerHnd,r.ul);

   LGadDrawBox(VB(&close_button),NULL);
}

//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayHackComp);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHackComp);

   // set up the continue button
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
void ShockComputerStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();
      //ShockOverlayChangeObj(kOverlayHRMPlug, kOverlayModeOn, ShockOverlayGetObj());

      ObjID o;
      o = ShockOverlayGetObj();
      if (ObjGetObjState(o) == kObjStateBroken)
         ShockHRMPlugSetMode(1, o, 0);
      else
         ShockHRMPlugSetMode(0, o, 0);
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
   }
}
//--------------------------------------------------------------------------------------
