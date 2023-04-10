/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkiftul.cpp,v 1.9 2000/02/19 13:25:29 toml Exp $

#include <2d.h>
#include <appagg.h>
#include <res.h>
#include <resapi.h>
#include <mprintf.h>
#include <schema.h>
#include <scrnmode.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

#include <shkutils.h>
#include <shkiftul.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern int gMFDRight;

#define NUM_NAV_BUTTONS 4
#define BDIM(x,y) {{x,y},{x+40,y+22}}
static Rect nav_rects[NUM_NAV_BUTTONS] = {
   BDIM(15,270), BDIM(55,270), BDIM(95,270), BDIM(135,270),
};
static LGadButtonList nav_blist;
static LGadButtonListDesc nav_blistdesc;
static DrawElement nav_blist_elems[NUM_NAV_BUTTONS];
static IRes *nav_handles[NUM_NAV_BUTTONS][2];
static grs_bitmap *nav_bitmaps[NUM_NAV_BUTTONS][4];

char *nav_names[NUM_NAV_BUTTONS] = { "estats", "etech", "ecmbt", "epsi" };
//--------------------------------------------------------------------------------------
void ShockMFDNavButtonsInit()
{
   int n,i;
   char temp[255];
   for (n=0; n < NUM_NAV_BUTTONS; n++)
   {
      for (i=0; i < 2; i++)
      {
         sprintf(temp,"%s%d",nav_names[n],i);
         nav_handles[n][i] = LoadPCX(temp); 
         nav_bitmaps[n][i] = (grs_bitmap *) nav_handles[n][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         nav_bitmaps[n][i] = nav_bitmaps[n][0];
      }
   }
}

//--------------------------------------------------------------------------------------
void ShockMFDNavButtonsTerm()
{
   int n;
   for (n=0; n < NUM_NAV_BUTTONS; n++)
   {
      nav_handles[n][0]->Unlock();
      nav_handles[n][1]->Unlock();
      SafeFreeHnd(&nav_handles[n][0]);
      SafeFreeHnd(&nav_handles[n][1]);
   }
}

//--------------------------------------------------------------------------------------
static bool nav_cb(ushort action, int button, void* data, LGadBox* vb)
{
   int which_mfd;   

   if (action == MOUSE_LDOWN)
      SchemaPlay((Label *)"bclick2",NULL);

   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   SchemaPlay((Label *)"btabs", NULL);

   switch (button)
   {
   case 0: which_mfd = kOverlayStats; break;
   case 1: which_mfd = kOverlayTechSkill; break;
   case 2: which_mfd = kOverlaySkills; break;
   case 3: which_mfd = kOverlayPsi; break;
   default:
      return(TRUE);
   }

   // set our "memory" of which MFD to open 
   gMFDRight = which_mfd;

   // this will automatically turn off any conflicting MFDs
   uiDefer(DeferOverlayOpen,(void *)which_mfd);
   
   //mprintf("nav button %d\n",button);
   return(TRUE);
}

//--------------------------------------------------------------------------------------
void ShockMFDNavButtonsBuild(Rect *r)
{
   int n;
   static Rect use_rects[NUM_NAV_BUTTONS];
	// make a button for each nav dest, ie a button list
   for (n=0; n < NUM_NAV_BUTTONS; n++)
   {
      DrawElement *elem = &nav_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = nav_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = nav_rects[n].ul.x + r->ul.x;
      use_rects[n].ul.y = nav_rects[n].ul.y + r->ul.y;
      use_rects[n].lr.x = nav_rects[n].lr.x + r->ul.x;
      use_rects[n].lr.y = nav_rects[n].lr.y + r->ul.y;
   }
   
   nav_blistdesc.num_buttons = NUM_NAV_BUTTONS;
   nav_blistdesc.button_rects = use_rects;
   nav_blistdesc.button_elems = nav_blist_elems;
   nav_blistdesc.cb = nav_cb;

   LGadCreateButtonListDesc(&nav_blist, LGadCurrentRoot(), &nav_blistdesc);
}

//--------------------------------------------------------------------------------------
void ShockMFDNavButtonsDestroy()
{
   LGadDestroyBox(VB(&nav_blist),FALSE);
}

//--------------------------------------------------------------------------------------
void ShockMFDNavButtonsDraw()
{
   LGadDrawBox(VB(&nav_blist),NULL);
}
//--------------------------------------------------------------------------------------

void SetLeftMFDRect(int which, Rect full_rect)
{
   sScrnMode smode;
   ScrnModeGet(&smode);

   if (smode.w == 640)
      ShockOverlaySetRect(which, full_rect);
   else
   {
      short w,h;
      Rect use_rect;
      w = RectWidth(&full_rect);
      h = RectHeight(&full_rect);
      use_rect.ul.x = full_rect.ul.x;
      if (smode.w >= 1024)
         use_rect.ul.y = full_rect.ul.y - LMFD_Y;
      else
         use_rect.ul.y = smode.h - LMFD_H - 56;
      use_rect.lr.x = use_rect.ul.x + w;
      use_rect.lr.y = use_rect.ul.y + h;
      ShockOverlaySetRect(which, use_rect);
   }
}

//--------------------------------------------------------------------------------------
void SetRightMFDRect(int which, Rect full_rect)
{
   sScrnMode smode;
   ScrnModeGet(&smode);

   if (smode.w == 640)
      ShockOverlaySetRect(which, full_rect);
   else
   {
      short w,h;
      Rect use_rect;
      w = RectWidth(&full_rect);
      h = RectHeight(&full_rect);
      use_rect.ul.x = smode.w - (640 - full_rect.ul.x);
      if (smode.w >= 1024)
         use_rect.ul.y = full_rect.ul.y - RMFD_Y;
      else
         use_rect.ul.y = smode.h - RMFD_H - 56;
      use_rect.lr.x = use_rect.ul.x + w;
      use_rect.lr.y = use_rect.ul.y + h;
      ShockOverlaySetRect(which, use_rect);
   }
}
//--------------------------------------------------------------------------------------
