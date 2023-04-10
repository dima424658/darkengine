/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhplug.cpp,v 1.7 2000/02/19 13:25:24 toml Exp $

// Keypad gump
#include <2d.h>
#include <appagg.h>
#include <res.h>

#include <resapi.h>
#include <kbcook.h>

#include <editor.h>

#include <scrptapi.h>
#include <shkscrm.h>
#include <scrnmode.h>

#include <shkprop.h>
#include <shkhplug.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkmfddm.h>
#include <shkhrm.h>
#include <shkiftul.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

IRes *gPlugBacks[3];
int gPlugMode = 0;
ObjID gPlugObj = OBJ_NULL;

static LGadButton plug_button;
static DrawElement plug_elem;
static IRes *plug_handles[3][2];
static grs_bitmap *plug_bitmaps[3][4];

#define PLUG_Y (LMFD_Y + 96) 
static Rect full_rect = {{181,PLUG_Y},{181 + 73, PLUG_Y + 194}};
static Rect plug_rect = {{16,114},{16 + 52,114 + 74}};

static int gPlugBonus = 0;

char *plug_base[3] = { "plughack", "plugrep", "plugmod" };

char *butt_plug_base[3] = { 
   "plugh", // the magic word
   "plugr", // but I just met her!
   "plugm", // but we just met them!
};
// sorry, I couldn't resist.

//--------------------------------------------------------------------------------------
void ShockHRMPlugInit(int which)
{
   char temp[40];
   int i,j;
   Rect use_rect;

   for (i=0; i<3; i++)
   {
      gPlugBacks[i] = LoadPCX(plug_base[i]);
      for (j=0; j < 2; j++)
      {
         sprintf(temp,"%s%d",butt_plug_base[i],j);
         plug_handles[i][j] = LoadPCX(temp);
         plug_bitmaps[i][j] = (grs_bitmap *) plug_handles[i][j]->Lock();
      }
      for (j=3; j<4; j++)
         plug_bitmaps[i][j] = plug_bitmaps[i][0];
   }

   sScrnMode smode;
   ScrnModeGet(&smode);

   // have some gruesome constants.
   // the 6 is just to offset us up a little so that we aren't falling off the end of the MFD
   // the 66 is to compensate for the health meters, including the little "bump" at the end
   // note that in 1024 mode we are handling for MFDs flush against top
   // and in 800 mode we are handling for MFDs snug against meters at bottom
   if (smode.w > 640)
   {
      use_rect.ul.x = full_rect.ul.x;
      if (smode.w >= 1024)
         use_rect.ul.y = LMFD_H - RectHeight(&full_rect) - 6;
      else
         use_rect.ul.y = smode.h - RectHeight(&full_rect) - 66;
      use_rect.lr.x = full_rect.ul.x + RectWidth(&full_rect);
      use_rect.lr.y = full_rect.ul.y + RectHeight(&full_rect);
      ShockOverlaySetRect(kOverlayHRMPlug, use_rect);
   }
   else
      ShockOverlaySetRect(kOverlayHRMPlug, full_rect);

}

//--------------------------------------------------------------------------------------
void ShockHRMPlugTerm(void)
{
   int s,i;

   for (s = 0; s < 3; s++)
   {
      SafeFreeHnd(&gPlugBacks[s]);

      for (i = 0; i < 2; i++)
      {
         plug_handles[s][i]->Unlock();
         SafeFreeHnd(&plug_handles[s][i]);
      }
   }
}

//--------------------------------------------------------------------------------------
void ShockHRMPlugDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHRMPlug);

   DrawByHandle(gPlugBacks[gPlugMode],r.ul);

   LGadDrawBox(VB(&plug_button),NULL);
}
//--------------------------------------------------------------------------------------
static void HackItBaby(void *)
{
   ShockHRMDisplay(gPlugObj, gPlugMode, FALSE, gPlugBonus);
}
//--------------------------------------------------------------------------------------
static bool plug_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   //uiDefer(DeferOverlayClose,(void *)kOverlayHRMPlug);
   uiDefer(HackItBaby,NULL);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHRMPlug);
   static Rect use_rect;

   // set up the continue button
   plug_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   plug_elem.draw_data = plug_bitmaps[gPlugMode];
   plug_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&plug_button, LGadCurrentRoot(), plug_rect.ul.x + r.ul.x, plug_rect.ul.y + r.ul.y,
      RectWidth(&plug_rect), RectHeight(&plug_rect), &plug_elem, plug_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&plug_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockHRMPlugStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
void ShockHRMPlugSetMode(int mode, ObjID obj, int bonus)
{
   gPlugMode = mode;
   gPlugObj = obj;
   gPlugBonus = bonus;
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockHRMPlugCheckTransp(Point pt)
{
   return(ShockOverlayCheckTransp(pt,kOverlayHRMPlug, gPlugBacks[gPlugMode]));
}
