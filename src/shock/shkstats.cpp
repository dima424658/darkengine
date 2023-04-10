/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkstats.cpp,v 1.26 2000/02/19 13:26:17 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>
#include <schema.h>
#include <scrnmode.h>

#include <playrobj.h>

#include <shkprop.h>
#include <shkstats.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkplprp.h>
#include <shkiftul.h>
#include <shktrait.h>
#include <shktrcst.h>
#include <shkmfddm.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

static IRes *gStatBack = NULL;
static IRes *gArrowLitHnd = NULL;
//static IRes *gArrowDimHnd = NULL;
static IRes *gArrowBoostHnd = NULL;
static IRes *gNavDownHnd = NULL;

static Rect full_rect = {{RMFD_X, RMFD_Y}, {RMFD_X + RMFD_W, RMFD_Y + RMFD_H}}; 
static Rect close_rect = {{5,8},{5 + 20, 8 + 21}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];
static Rect trait_rect = {{15,148},{174,184}};

//--------------------------------------------------------------------------------------
void ShockStatsInit(int which)
{
   int i;
   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   gStatBack = LoadPCX("stats");
   //gArrowDimHnd = LoadPCX("arrow0");
   gArrowLitHnd = LoadPCX("skilstat");
   gArrowBoostHnd = LoadPCX("statboos");
   gNavDownHnd = LoadPCX("estats1");

   SetRightMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockStatsTerm(void)
{
   SafeFreeHnd(&gStatBack);
   SafeFreeHnd(&gArrowLitHnd);
   //SafeFreeHnd(&gArrowDimHnd);
   SafeFreeHnd(&gArrowBoostHnd);
   SafeFreeHnd(&gNavDownHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
#define ARROW_X   33
#define ARROW_Y   22
#define ARROW_DX  17
#define ARROW_DY  26

#define NAME_X    32
#define NAME_Y    8
#define LEVEL_X   158
#define LEVEL_Y   8
#define EXP_X     32
#define EXP_Y     23
#define SERVICE_X 104
#define SERVICE_Y 23

#define TRAIT_X   37
#define TRAIT_Y   144
#define TRAIT_W   35

static Rect stat_rects[] = {
   // 0
   {{30,8},{178,34}}, // STR
   {{30,35},{178,60}}, // END
   {{30,61},{178,86}}, // PSI
   {{30,87},{178,112}}, // AGI
   {{30,113},{178,143}}, // CYB
   // 5
   {{30,7},{124,19}}, // name
   {{125,7},{178,19}}, // level
   {{30,20},{94,52}}, // exp
   {{30,123},{178,143}}, // build pts
   {{10,266},{176,291}}, // nav buttons
   //10
   {{34,148},{68,183}}, // trait 1
   {{69,148},{104,183}}, // trait 2
   {{105,148},{140,183}}, // trait 3
   {{141,148},{177,183}}, // trait 4
};
#define NUM_STAT_RECTS (sizeof(stat_rects) / sizeof(Rect))
static Rect text_rect = {{15,190},{174,264}};
#define BDIM(x,y) {{x,y},{x+39,y+17}}
static Rect navdown_rect = BDIM(15,270);

//--------------------------------------------------------------------------------------
static int FindStatSlot(Point mpt)
{
   int retval = -1;
   int i;

   for (i=0; i < NUM_STAT_RECTS; i++)
   {
      if (RectTestPt(&stat_rects[i], mpt))
      {
         retval = i;
         break;
      }
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
static void DrawMouseOverText(ObjID obj, Rect r)
{
   Point mpt;
   int i;
   char temp[255];
   int dx,dy;
   char name[32];
   char table[32];
   strcpy(table,"");
   strcpy(name,"");
   int strnum = 0;

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   i = FindStatSlot(mpt);
   if (i != -1)
   {
      if ((i >= 10) && (i <= 13))
      {
         sTraitsDesc *pTrait;
         int t;

         if (g_TraitsProperty->Get(obj, &pTrait))
         {
            t = pTrait->m_traits[i - 10];
            // resourceify this
            if (t == -1)
            {
               strcpy(name,"NoTrait");
               strcpy(table,"traits");
               strnum = -1;
            }
            else
            {
               strcpy(name,"trait");
               strcpy(table,"traits");
               strnum = t;
            }
         }
      }
      else
      {
         strcpy(name,"text");
         strcpy(table,"stathelp");
         strnum = i;
      }
      ShockStringFetch(temp,sizeof(temp),name,table,strnum);
      gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rect));
      dx = r.ul.x + text_rect.ul.x; //+ (RectWidth(&text_rect) - w) / 2;
      dy = r.ul.y + text_rect.ul.y; //+ (RectHeight(&text_rect) - h) / 2;
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFont,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------
// boy, this function is getting ugly.
// maybe it should get split out?
void ShockStatsDraw(void)
{
   Rect r;
   sStatsDesc *pstats, modstats;
   //char temp[255];
   int dx,dy;
   int i,j;
   Point drawpt;
   //int val;
   ObjID obj;
   sTraitsDesc *pTrait;
   AutoAppIPtr(ShockPlayer);

   obj = PlayerObject();
   
   // draw the background
   r = ShockOverlayGetRect(kOverlayStats);
   DrawByHandle(gStatBack,r.ul);

   gr_set_fcolor(gShockTextColor);

   // draw in the traits (real and possible)
   if (g_TraitsProperty->Get(obj, &pTrait))
   {
      dx = TRAIT_X + r.ul.x;
      dy = TRAIT_Y + r.ul.y;
      for (i=0; i < NUM_TRAIT_SLOTS; i++)
      {
         if (pTrait->m_traits[i] > 0)
         {
            ShockTraitDrawIcon((eTrait)pTrait->m_traits[i],dx,dy);
            dx = dx + TRAIT_W;
         }
      }
   }

   /*
   if (g_ServiceProperty->Get(obj, &val))
   {
      ShockStringFetch(temp,sizeof(temp),"ServiceName","misc",val);
      dx = r.ul.x + SERVICE_X;
      dy = r.ul.y + SERVICE_Y;
      gr_font_string(gShockFont, temp, dx, dy);
   }
   */

   // draw in the arrows for current stat levels & potential
   // note that we are directly accessing the property here since we
   // want to know your BASE stats, not your post-effects stat
   if (g_BaseStatsProperty->Get(obj, &pstats))
   {
      pShockPlayer->GetStats(obj, &modstats);
      drawpt.y = ARROW_Y + r.ul.y;
      for (i=kStatStrength; i <= kStatCyber; i++)
      {

         drawpt.x = ARROW_X + r.ul.x;
         // this will need reworking if modstats can be less than base stats
         for (j=0; j < MAX_STAT_VAL; j++)
         {
            if (j < pstats->m_stats[i])
               DrawByHandle(gArrowLitHnd, drawpt);
            else if (j < modstats.m_stats[i])
               DrawByHandle(gArrowBoostHnd, drawpt);
            //else if (j < modstats.m_stats[i] + buy)
            //   DrawByHandle(gArrowDimHnd, drawpt);
            else
               break;
            drawpt.x = drawpt.x + ARROW_DX;
         }
         drawpt.y = drawpt.y + ARROW_DY;
      }
   }

   // draw the current stat description
   DrawMouseOverText(obj,r);

   // draw in misc nav type buttons
   LGadDrawBox(VB(&close_button),NULL);
   ShockMFDNavButtonsDraw();
   // draw on top of the nav button our "down" state
   drawpt.x = navdown_rect.ul.x + r.ul.x;
   drawpt.y = navdown_rect.ul.y + r.ul.y;
   DrawByHandle(gNavDownHnd, drawpt);
}
//--------------------------------------------------------------------------------------
EXTERN int hack_for_kbd_state(void);
#define CTRL_STATES  (KBM_LCTRL|KBM_RCTRL)
bool ShockStatsHandleMouse(Point mpt)
{
   Rect r = ShockOverlayGetRect(kOverlayStats);
   ObjID obj;
   AutoAppIPtr(ShockPlayer);

   if (shock_cursor_mode != SCM_NORMAL) 
      return(TRUE);

   obj = PlayerObject();

   /*
   // click on trait button
   // traits moved to a machine -- Xemu
   if (RectTestPt(&trait_rect,mpt))
   {
      uiDefer(DeferOverlayOpen,(void *)kOverlayBuyTraits);
   }
   */

   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      SchemaPlay((Label *)"subpanel_cl",NULL);
      uiDefer(DeferOverlayClose,(void *)kOverlayStats);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayStats);
   ShockMFDNavButtonsBuild(&r);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   ShockMFDNavButtonsDestroy();

   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockStatsStateChange(int which)
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
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockStatsCheckTransp(Point pt)
{
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
   return(FALSE);
}


