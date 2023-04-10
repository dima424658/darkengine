/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkscomp.cpp,v 1.6 2000/02/19 13:26:13 toml Exp $

// alarm icon
#include <2d.h>
#include <appagg.h>

#include <resapi.h>
#include <playrobj.h>
#include <linkman.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>

#include <shkscomp.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkhplug.h>
#include <shkobjst.h>
#include <shkiftul.h>
#include <shkprop.h>
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

static IRes *gSecCompHnd;
static IRes *gBrokenHnd;

//--------------------------------------------------------------------------------------
void ShockSecCompInit(int which)
{
   int i;

   gSecCompHnd= LoadPCX("alarmfd");
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
void ShockSecCompTerm(void)
{
   SafeFreeHnd(&gSecCompHnd);
   SafeFreeHnd(&gBrokenHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
#define TEXT_X 18
#define TEXT_Y 187
#define TEXT_W 150
void ShockSecCompDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlaySecurComp);
   ObjID o = ShockOverlayGetObj();
   eObjState st;
   st = ObjGetObjState(o);

   if (st == kObjStateBroken)
      DrawByHandle(gBrokenHnd,r.ul);
   else
   {
      DrawByHandle(gSecCompHnd,r.ul);

      // draw in state text
      char temp[255];
      int dx,dy;

      // to figure out mode, we ask a few questions
      int mode = 1;

      if (ShockOverlayCheck(kOverlayHackIcon) == kOverlayModeOn)
         mode = 0;
      /*
      else
      {
         // okay, are we linked to a camera in an alarm state?
         IRelation *pRel;
         ILinkQuery *query;
         AutoAppIPtr(LinkManager);
         ObjID cam;
         BOOL isalarm = FALSE;
         sLink link;

         pRel = pLinkManager->GetRelationNamed("SwitchLink");
         query = pRel->Query(o, LINKOBJ_WILDCARD);
         while (!query->Done())
         {
            query->Link(&link);
            cam = link.dest;
            gPropAlarm->Get(cam,&isalarm);
            if (isalarm)
               mode = 2;
            query->Next();
         }
         SafeRelease(query);
      }
      */

      ShockStringFetch(temp,sizeof(temp), "AlarmState", "misc", mode);
      gr_font_string_wrap(gShockFont,temp,TEXT_W);
      dx = r.ul.x + TEXT_X;
      dy = r.ul.y + TEXT_Y;
      gr_font_string(gShockFont,temp,dx,dy);
   }

   LGadDrawBox(VB(&close_button),NULL);
}

//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlaySecurComp);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlaySecurComp);

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
void ShockSecCompStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();

      ObjID o;
      o = ShockOverlayGetObj();
      int bonus;
      AutoAppIPtr(ShockPlayer);
      if (pShockPlayer->HasTrait(PlayerObject(),kTraitSecurity))
         bonus = 2;
      else
         bonus = 0;
      if (ObjGetObjState(o) == kObjStateBroken)
         ShockHRMPlugSetMode(1, o, 0);
      else
         ShockHRMPlugSetMode(0, o, bonus);
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
