/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsamp.cpp,v 1.9 2000/02/19 13:25:59 toml Exp $

#if 0

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>
#include <schema.h>

#include <playrobj.h>

#include <shkprop.h>
//#include <shkscrpt.h>
#include <shkpsamp.h>
#include <shkpsapi.h>
#include <shkpsiif.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkplprp.h>
#include <shkiftul.h>
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

static int gPsiLevel = 0;

static IRes *gPsiBack = NULL;

static IRes *gLevelHandles[NUM_PSI_LEVELS];

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

#define MAIN_DX   69
#define MAIN_DY   30

#define POOL_X    116
#define POOL_Y    166
//--------------------------------------------------------------------------------------
void ShockPsiAmpInit(int which)
{
   int i;
   char temp[255];

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   for (i=0; i < NUM_PSI_LEVELS; i++)
   {
      sprintf(temp,"psi%d",i+1);
      gLevelHandles[i] = LoadPCX(temp);
   }

   gPsiBack = LoadPCX("psiamp");
   ShockOverlaySetRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockPsiAmpTerm(void)
{
   int i;
   SafeFreeHnd(&gPsiBack);
   for (i=0; i < NUM_PSI_LEVELS; i++)
   {
      SafeFreeHnd(&gLevelHandles[i]);
   }

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------

static Rect stat_rects[] = {
   {{13,11},{155,28}}, // level select
   {{13,32},{155,158}}, // powers area
};
#define NUM_STAT_RECTS (sizeof(stat_rects) / sizeof(Rect))
static Rect main_rect = {{13,32},{155,158}};
static Rect level_rect = {{13,11},{155,28}};
static Rect text_rect = {{13,162},{172,236}};

//--------------------------------------------------------------------------------------
static int FindSlot(Point mpt)
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
static int GetPsiPower(Point mpt)
{
   int x,y;
   int retval = -1;
   x = (mpt.x - main_rect.ul.x) / MAIN_DX;
   y = (mpt.y - main_rect.ul.y) / MAIN_DY;
   // do we have the current level?
   AutoAppIPtr(ShockPlayer);
   retval = psi_levels[gPsiLevel];
   retval = retval + x;
   retval = retval + (y * 2);
   return(retval);
}
//--------------------------------------------------------------------------------------
static void DrawMouseOverText(ObjID obj)
{
   Point mpt;
   int i;
   int power;
   char temp[255];
   int dx,dy;
   Rect r = ShockOverlayGetRect(kOverlayPsiAmp);

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   i = FindSlot(mpt);
   if (i != -1)
   {
      if (i == 1)
      {
         power = GetPsiPower(mpt);
         if (power == -1)
         {
            return;
         }
         ShockStringFetch(temp,sizeof(temp),"psi","psihelp",power);
      }
      else
      {
         ShockStringFetch(temp,sizeof(temp),"amptext","psihelp",i);
      }
      gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rect));
      dx = r.ul.x + text_rect.ul.x; 
      dy = r.ul.y + text_rect.ul.y; 
      gr_font_string(gShockFont,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------

#define PSI_X1    13
#define PSI_X2    82
#define PSI_Y     34
#define PSI_DY    30

void ShockPsiAmpDraw(void)
{
   Rect r;
   int dx,dy;
   Point drawpt;
   ObjID obj;
   int power,curpower;
   int kind;

   AutoAppIPtr(ShockPlayer);
   obj = PlayerObject();

   // draw the background
   r = ShockOverlayGetRect(kOverlayPsiAmp);
   DrawByHandle(gPsiBack,r.ul);

   // draw currently selected level
   drawpt.x = level_rect.ul.x + r.ul.x;
   drawpt.y = level_rect.ul.y + r.ul.y;
   //mprintf("psilevel = %d\n",gPsiLevel);
   DrawByHandle(gLevelHandles[gPsiLevel],drawpt);

   // draw the psi powers for the current level
   AutoAppIPtr(PlayerPsi);
   curpower = pPlayerPsi->GetSelectedPower();
   power = psi_levels[gPsiLevel];
   dx = PSI_X1 + r.ul.x;
   dy = PSI_Y + r.ul.y;
   // okay to do this since psi_levels is padded one high
   while (power < psi_levels[gPsiLevel+1])
   {
      if (curpower == power)
         kind = 2;
      else if (pShockPlayer->PsiPower(obj, (ePsiPowers)power))
         kind = 1;
      else
         kind = 0;
      ShockDrawPsiIcon((ePsiPowers)power,dx,dy,kind);

      // advance
      if (dx == PSI_X1 + r.ul.x)
         dx = PSI_X2 + r.ul.x;
      else
      {
         dx = PSI_X1 + r.ul.x;
         dy = dy + PSI_DY;
      }
      power = power + 1;
   }

   LGadDrawBox(VB(&close_button),NULL);

   // draw the current stat description
   DrawMouseOverText(obj);

}
//--------------------------------------------------------------------------------------
bool ShockPsiAmpHandleMouse(Point mpt)
{
   Rect r = ShockOverlayGetRect(kOverlayPsiAmp);
   ObjID obj;
   int slot,power;
   AutoAppIPtr(ShockPlayer);

   if (shock_cursor_mode != SCM_NORMAL) 
      return(TRUE);

   obj = PlayerObject();

   if (RectTestPt(&level_rect,mpt))
   {
      int dx = RectWidth(&level_rect) / NUM_PSI_LEVELS;
      int newlevel = (mpt.x - level_rect.ul.x) / dx;
      if (newlevel < 0) newlevel = 0;
      if (newlevel >= NUM_PSI_LEVELS)
         newlevel = NUM_PSI_LEVELS - 1;
      gPsiLevel = newlevel;
   }
   // click on arrows
   slot = FindSlot(mpt);
   if (slot == 1)
   {
      power = GetPsiPower(mpt);
      if (power == -1)
         return(TRUE);

      // if we have the power already, then set it
      if (pShockPlayer->PsiPower(obj,(ePsiPowers)power))
      {
         // set it
         AutoAppIPtr(PlayerPsi);
         pPlayerPsi->Select((ePsiPowers)power);
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);
   uiDefer(DeferOverlayClose,(void *)kOverlayPsiAmp);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayPsiAmp);

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
void ShockPsiAmpStateChange(int which)
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
bool ShockPsiAmpCheckTransp(Point pt)
{
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
   return(FALSE);
}

#endif