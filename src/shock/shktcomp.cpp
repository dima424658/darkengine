/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktcomp.cpp,v 1.2 2000/02/19 13:26:22 toml Exp $

// turret MFD
#include <2d.h>
#include <appagg.h>

#include <resapi.h>
#include <playrobj.h>
#include <aiteams.h>
#include <aiprcore.h>

#include <shktcomp.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkhplug.h>
#include <shkobjst.h>
#include <shkiftul.h>
#include <shkprop.h>

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

static IRes *gTurretHnd;

//--------------------------------------------------------------------------------------
void ShockTurretInit(int which)
{
   int i;

   gTurretHnd= LoadPCX("turrmfd");

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
void ShockTurretTerm(void)
{
   SafeFreeHnd(&gTurretHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
#define TEXT_X 18
#define TEXT_Y 187
#define TEXT_W 150
void ShockTurretDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlaySecurComp);
   ObjID o = ShockOverlayGetObj();
   eObjState st;
   st = ObjGetObjState(o);

   DrawByHandle(gTurretHnd,r.ul);

   // draw in state text
   char temp[255];
   int dx,dy;

   // to figure out mode, we ask a few questions
   int mode = 2;

   if (AIGetTeam(o) == 0)
      mode = 0;
   else if (ShockOverlayCheck(kOverlayHackIcon) == kOverlayModeOn)
      mode = 1;

   // 0: hacked to your side  1: world is hacked   2: normal
   ShockStringFetch(temp,sizeof(temp), "TurretState", "misc", mode);
   gr_font_string_wrap(gShockFont,temp,TEXT_W);
   dx = r.ul.x + TEXT_X;
   dy = r.ul.y + TEXT_Y;
   gr_font_string(gShockFont,temp,dx,dy);

   LGadDrawBox(VB(&close_button),NULL);
}

//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayTurret);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayTurret);

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
void ShockTurretStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();

      ObjID o;
      o = ShockOverlayGetObj();
      if (AIGetTeam(o) != 0)
      {
         ShockHRMPlugSetMode(0, o);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
   }
}
//--------------------------------------------------------------------------------------
