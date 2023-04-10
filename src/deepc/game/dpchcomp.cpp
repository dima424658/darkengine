/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// alarm icon
#include <2d.h>

#include <resapi.h>
#include <appagg.h>

#include <playrobj.h>

#include <dpchcomp.h>
#include <dpcovrly.h>
#include <dpcutils.h>
#include <dpcovcst.h>
#include <dpcmfddm.h>
#include <dpchplug.h>
#include <dpcobjst.h>
#include <dpcprop.h>
#include <dpciftul.h>
#include <dpcplayr.h>
#include <dpctrcst.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
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
void DPCComputerInit(int which)
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
void DPCComputerTerm(void)
{
   SafeFreeHnd(&gComputerHnd);
   SafeFreeHnd(&gBrokenHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
void DPCComputerDraw(unsigned long inDeltaTicks)
{
   Rect r = DPCOverlayGetRect(kOverlayHackComp);
   eObjState st;
   ObjID o = DPCOverlayGetObj();
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
   Rect r = DPCOverlayGetRect(kOverlayHackComp);

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
void DPCComputerStateChange(int which)
{
   if (DPCOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();
      //DPCOverlayChangeObj(kOverlayHRMPlug, kOverlayModeOn, DPCOverlayGetObj());

      ObjID o;
      o = DPCOverlayGetObj();
      if (ObjGetObjState(o) == kObjStateBroken)
         DPCHRMPlugSetMode(1, o, 0);
      else
         DPCHRMPlugSetMode(0, o, 0);
      DPCOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
      DPCOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
   }
}
//--------------------------------------------------------------------------------------
