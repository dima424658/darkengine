/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsiif.cpp,v 1.25 2000/02/19 13:26:00 toml Exp $

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
//#include <shkscrpt.h>
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
#include <shkpsapi.h>
#include <shkmfddm.h>
#include <shkqbind.h>

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
static IRes *gNavDownHnd = NULL;

IRes *gLevelHandles[NUM_PSI_LEVELS];
IRes *gLevelBlocked[NUM_PSI_LEVELS];

static Rect full_rect = {{RMFD_X, RMFD_Y}, {RMFD_X + RMFD_W, RMFD_Y + RMFD_H}}; 
static Rect close_rect = {{5,8},{5 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

ePsiPowers psi_levels[NUM_PSI_LEVELS+1] = { kPsiLevel1, kPsiLevel2, kPsiLevel3, kPsiLevel4, kPsiLevel5,kPsiMax };

#define MAIN_DX   69
#define MAIN_DY   30

#define LEVEL_X   32
#define LEVEL_Y   11

#define PSI_X1    36 
#define PSI_X2    105
#define PSI_Y     34
#define PSI_DY    30

#define POOL_X    116
#define POOL_Y    166
//--------------------------------------------------------------------------------------
void ShockPsiInit(int which)
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
      if (i > 0)
      {
         sprintf(temp,"psi%dblok",i+1);
         gLevelBlocked[i] = LoadPCX(temp);
      }
   }

   gPsiBack = LoadPCX("psi");
   gNavDownHnd = LoadPCX("epsi1");

   SetRightMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockPsiTerm(void)
{
   int i;
   SafeFreeHnd(&gPsiBack);
   SafeFreeHnd(&gNavDownHnd);
   for (i=0; i < NUM_PSI_LEVELS; i++)
   {
      SafeFreeHnd(&gLevelHandles[i]);
      if (i > 0)
         SafeFreeHnd(&gLevelBlocked[i]);
   }

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------

// this will get turned into a string resource
static Rect stat_rects[] = {
   {{32,10},{174,28}}, // level select
   {{32,32},{174,157}}, // powers area
   {{-1,-1},{-1,-1}}, // points
   {{10,267},{175,290}}, // nav buttons
};
#define NUM_STAT_RECTS (sizeof(stat_rects) / sizeof(Rect))
static Rect main_rect = {{36,43},{174,154}};
static Rect level_rect = {{32,10},{174,28}};
static Rect text_rect = {{15,162},{174,264}};
#define BDIM(x,y) {{x,y},{x+39,y+17}}
static Rect navdown_rect = BDIM(135,270);

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
   Rect r;
   int retval = -1;

   r = ShockOverlayGetRect(kOverlayPsi);

   if (mpt.x  > PSI_X2)
      x = 1;
   else
      x = 0;

   y = (mpt.y - PSI_Y) / PSI_DY;
   if (y > 3)
      y = 3;
   //mprintf("x = %d, y = %d (%d - %d - %d / %d)\n",x,y,mpt.y,PSI_Y,r.ul.y,PSI_DY);

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
   Rect r;

   r = ShockOverlayGetRect(kOverlayPsi);

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
         ShockStringFetch(temp,sizeof(temp),"buytext","psihelp",i);
      }
      gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rect));
      dx = r.ul.x + text_rect.ul.x; 
      dy = r.ul.y + text_rect.ul.y; 
      //gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFont,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------
void ShockDrawPsiIcon(ePsiPowers which, int dx, int dy, int kind)
{
   char temp[255],temp2[250];
   IRes *hnd;
   grs_bitmap *bmp;

   if ((which >= kPsiMax) || (which < kPsiLevel1))
      return;

   //sprintf(temp,"picn%02d_%d",which,kind);
   ShockStringFetch(temp2,sizeof(temp2),"PsiIcon","psihelp",which);
   sprintf(temp,"%s_%d",temp2,kind);
   // find the bitmap
   hnd = LoadPCX(temp);
   if (hnd != NULL)
   {
      // draw it!
      bmp = (grs_bitmap *) hnd->Lock();
      gr_bitmap(bmp, dx, dy);
      hnd->Unlock();

      // cleanup
      SafeFreeHnd(&hnd);
   }
}
//--------------------------------------------------------------------------------------

//#define AMP_X1    15
//#define AMP_X2    94

void ShockPsiDraw(void)
{
   Rect r;
   int dx,dy;
   Point drawpt;
   ObjID obj;
   int power;
   int kind;
   int curpower;

   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(PlayerPsi);
   obj = PlayerObject();
   
   // draw the background
   r = ShockOverlayGetRect(kOverlayPsi);
   DrawByHandle(gPsiBack,r.ul);

   sStatsDesc *basestats;
   if (!g_BaseStatsProperty->Get(obj, &basestats))
      return;

   // draw currently selected level
   drawpt.x = LEVEL_X + r.ul.x;
   drawpt.y = LEVEL_Y + r.ul.y;
   //mprintf("psilevel = %d\n",gPsiLevel);
   DrawByHandle(gLevelHandles[gPsiLevel],drawpt);

   /*
   if (basestats->m_stats[kStatPsi] < 5)
   {
      DrawByHandle(gLevelBlocked[basestats->m_stats[kStatPsi]], drawpt);
   }
   */

   // draw the psi powers for the current level
   power = psi_levels[gPsiLevel];
   dx = PSI_X1 + r.ul.x;
   dy = PSI_Y + r.ul.y;

   curpower = pPlayerPsi->GetSelectedPower();

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

      // draw quickbind letter
      int bind = ShockBindVal((ePsiPowers)power);
      if (bind > 0)
      {
         char temp[32];
         sprintf(temp,"F%d",bind);
         gr_font_string(gShockFont,temp,dx,dy);
      }

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

   // draw the current stat description
   DrawMouseOverText(obj);

   // draw in misc nav type buttons
   LGadDrawBox(VB(&close_button),NULL);
   ShockMFDNavButtonsDraw();

   // draw on top of the nav button our "down" state
   drawpt.x = navdown_rect.ul.x + r.ul.x;
   drawpt.y = navdown_rect.ul.y + r.ul.y;
   //mprintf("drawing at %d, %d\n",drawpt.x,drawpt.y);
   DrawByHandle(gNavDownHnd, drawpt);
}
//--------------------------------------------------------------------------------------
bool ShockPsiHandleMouse(Point mpt)
{
   Rect r = ShockOverlayGetRect(kOverlayPsi);
   ObjID obj;
   int slot, power;
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
      SchemaPlay((Label *)"btabs",NULL);
   }

   // click on arrows
   slot = FindSlot(mpt);
   if (slot == 1)
   {
      power = GetPsiPower(mpt);
      if (power == -1)
         return(TRUE);

      if (pShockPlayer->IsPsiTier((ePsiPowers)power))
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
   if (action == BUTTONGADG_LCLICK)
   {
      SchemaPlay((Label *)"subpanel_cl",NULL);
      uiDefer(DeferOverlayClose,(void *)kOverlayPsi);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayPsi);
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
void ShockPsiStateChange(int which)
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
bool ShockPsiCheckTransp(Point pt)
{
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
   return(FALSE);
}

//--------------------------------------------------------------------------------------
ePsiPowers ShockPsiQuickbind(void)
{
   Point rawpt, mpt;
   int power, slot;
   Rect r = ShockOverlayGetRect(kOverlayPsi);

   mouse_get_xy(&rawpt.x,&rawpt.y);
   mpt.x = rawpt.x - r.ul.x;
   mpt.y = rawpt.y - r.ul.y;

   slot = FindSlot(mpt);
   if (slot != 1)
      return(kPsiNone);

   // what psi power is this?
   power = GetPsiPower(mpt);
   return((ePsiPowers)power);
}
//--------------------------------------------------------------------------------------
void ShockPsiIconInit(int which)
{
   //gPsiIconHnd= LoadPCX("PsiIcon");
}

//--------------------------------------------------------------------------------------
void ShockPsiIconTerm(void)
{
   //SafeFreeHnd(&gPsiIconHnd);
}

//--------------------------------------------------------------------------------------
#define PSIICON_OFFX (66 + 10)
#define PSIICON_Y 132
#define PSIICON_DX  66
#define PSIICON_DY  40
#define PSIICON_COLUMN  6 
void ShockPsiIconDraw(void)
{
   AutoAppIPtr(PlayerPsi);
   IPsiActiveIter *pPsiIter;
   char temp[255];
   short dx, dy, w, x, y;
   int count;

   // find all the active psi icons & draw 'em!
   // how slow is this to do each frame?  Should we cache it?
   pPsiIter = pPlayerPsi->Iter();
   if (pPsiIter == NULL)
      return;

   sScrnMode smode;
   ScrnModeGet(&smode);

   dx = smode.w - PSIICON_OFFX;
   dy = PSIICON_Y;
   count = 0;
   while (!pPsiIter->Done())
   {
      ShockDrawPsiIcon(pPsiIter->GetPower(), dx, dy, 2);

      ShockStringFetch(temp,sizeof(temp),"PsiShort","psihelp",pPsiIter->GetPower());
      w = gr_font_string_width(gShockFontMono,temp);
      x = dx + ((PSIICON_DX - w) / 2);
      y = dy + PSIICON_DY - 13;
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFontMono, temp, x, y);
      
      dy = dy + PSIICON_DY;
      count++;
      if (count > PSIICON_COLUMN)
      {
         dx = dx - (PSIICON_OFFX);
         dy = PSIICON_Y;
         count = 0;
      }
      pPsiIter->Next();
   }
   SafeRelease(pPsiIter);
}
//--------------------------------------------------------------------------------------
void ShockPsiRefocus(ePsiPowers power)
{
   AutoAppIPtr(ShockPlayer);
   int level = pShockPlayer->FindPsiLevel(power);
   gPsiLevel = level - 1;
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayPsi = { 
   ShockPsiDraw, // draw
   ShockPsiInit, // init
   ShockPsiTerm, // term
   ShockPsiHandleMouse, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   ShockPsiStateChange, // state
   ShockPsiCheckTransp, // transparency
   0, // distance
   TRUE, // needmouse
};

sOverlayFunc OverlayPsiIcon = { 
   ShockPsiIconDraw, // draw
   ShockPsiIconInit, // init
   ShockPsiIconTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   NULL, // state
   NULL, // transparency
   0, // distance
   FALSE, // needmouse
};
