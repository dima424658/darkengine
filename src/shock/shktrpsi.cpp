/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktrpsi.cpp,v 1.19 2000/02/19 13:26:29 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>
#include <schema.h>
#include <scrnmode.h>
#include <filevar.h>

#include <playrobj.h>

#include <shkprop.h>
//#include <shkscrpt.h>
#include <shkpsiif.h>
#include <shktrpsi.h>
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

// Here's the type of my global 
struct sPsiTrainData
{
   int m_level; 
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gPsiTrainDataDesc = 
{
   kCampaignVar,         // Where do I get saved?
   "PSITRAIN",          // Tag file tag
   "Psi Trainer Data",     // friendly name
   FILEVAR_TYPE(sPsiTrainData),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sPsiTrainData,&gPsiTrainDataDesc> gPsiTrainData; 


static IRes *gPsiBack = NULL;
static IRes *gNavDownHnd = NULL;
static IRes *gButtonHnd = NULL;
static IRes *gBigButtonHnd = NULL;

static int gUndoPool;
static int gUndoPowers;
static int gUndoPowers2;

extern IRes *gLevelHandles[NUM_PSI_LEVELS];
extern IRes *gLevelBlocked[NUM_PSI_LEVELS];

static Rect full_rect= {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static Rect undo_rect = {{158,146},{158 + 18, 146 + 44}}; // undo

static LGadButton undo_button;
static DrawElement undo_elem;
static IRes *undo_handles[2];
static grs_bitmap *undo_bitmaps[4];

extern ePsiPowers psi_levels[NUM_PSI_LEVELS+1];

#define MAIN_DX   69
#define MAIN_DY   30

#define POOL_X    68 
#define POOL_Y    167

#define LEVEL_X   12
#define LEVEL_Y   11

#define PSI_X1    15 
#define PSI_X2    80
#define PSI_Y     34
#define PSI_DY    30

//#define AMP_X1    15
//#define AMP_X2    94
//--------------------------------------------------------------------------------------
void ShockBuyPsiInit(int which)
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

   undo_handles[0] = LoadPCX("undo0"); 
   undo_handles[1] = LoadPCX("undo1"); 
   undo_bitmaps[0] = (grs_bitmap *) undo_handles[0]->Lock();
   undo_bitmaps[1] = (grs_bitmap *) undo_handles[1]->Lock();

   for (i = 2; i < 4; i++)
   {
      undo_bitmaps[i] = undo_bitmaps[0];
   }

   gPsiBack = LoadPCX("psitrain");
   gButtonHnd = LoadPCX("psit10");
   gBigButtonHnd = LoadPCX("psibuy10");

   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockBuyPsiTerm(void)
{
   SafeFreeHnd(&gPsiBack);
   SafeFreeHnd(&gButtonHnd);
   SafeFreeHnd(&gBigButtonHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   undo_handles[0]->Unlock();
   undo_handles[1]->Unlock();
   SafeFreeHnd(&undo_handles[0]);
   SafeFreeHnd(&undo_handles[1]);
}

//--------------------------------------------------------------------------------------

// this will get turned into a string resource
static Rect stat_rects[] = {
   {{12,11},{154,28}}, // level select
   {{12,32},{154,157}}, // powers area
   {{12,165},{154,182}}, // points
};

#define NUM_STAT_RECTS (sizeof(stat_rects) / sizeof(Rect))
//static Rect main_rect = {{16,43},{154,154}};
static Rect level_rect = {{12,10},{154,28}};
static Rect text_rect = {{12,190},{171,287}};

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
   if (mpt.x >= PSI_X2)
      x = 1;
   else
      x = 0;
   
   y = (mpt.y - PSI_Y) / PSI_DY;
   if (y > 3)
      y = 3;

   // do we have the current level?
   AutoAppIPtr(ShockPlayer);
   retval = psi_levels[gPsiTrainData.m_level];
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

   r = ShockOverlayGetRect(kOverlayBuyPsi);

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
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFont,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------
void ShockBuyPsiDraw(void)
{
   Rect r;
   char temp[255];
   int dx,dy;
   Point drawpt;
   ObjID obj;
   int pool;
   int power;
   int kind;
   int cost;

   AutoAppIPtr(ShockPlayer);

   obj = PlayerObject();
   
   // draw the background
   r = ShockOverlayGetRect(kOverlayBuyPsi);
   DrawByHandle(gPsiBack,r.ul);

   sStatsDesc *basestats;
   if (!g_BaseStatsProperty->Get(obj, &basestats))
      return;

   gr_set_fcolor(gShockTextColor);

   // draw currently selected level
   drawpt.x = LEVEL_X + r.ul.x;
   drawpt.y = LEVEL_Y + r.ul.y;
   //mprintf("psilevel = %d\n",gPsiTrainData.m_level);
   DrawByHandle(gLevelHandles[gPsiTrainData.m_level],drawpt);

   /*
   if ((basestats->m_stats[kStatPsi] < 5) && (!pShockPlayer->HasTrait(PlayerObject(), kTraitPsionic)))
   {
      DrawByHandle(gLevelBlocked[basestats->m_stats[kStatPsi]], drawpt);
   }
   */

   // draw in point pool
   // save off explicitly since being used again to compute possible buy
   pool = pShockPlayer->GetPool(obj);
   sprintf(temp,"%d",pool);
   dx = r.ul.x + POOL_X;
   dy = r.ul.y + POOL_Y;
   gr_font_string(gShockFont,temp,dx,dy);

   ShockStringFetch(temp,sizeof(temp),"TrainPoints","misc");
   dx = r.ul.x + 16;
   dy = r.ul.y + 167;
   gr_font_string(gShockFont,temp,dx,dy);

   // draw the psi powers for the current level
   power = psi_levels[gPsiTrainData.m_level];
   dx = PSI_X1 + r.ul.x;
   dy = PSI_Y + r.ul.y;

   // okay to do this since psi_levels is padded one high
   BOOL draw_border = FALSE; 
   while (power < psi_levels[gPsiTrainData.m_level+1])
   {
      cost = pShockPlayer->PsiPowerCost(power);
      if (power == psi_levels[gPsiTrainData.m_level])
      {
         if (pShockPlayer->PsiLevel(obj,gPsiTrainData.m_level))
            kind = 2;
         else if (pool < cost) //((gPsiTrainData.m_level + 1 > basestats->m_stats[kStatPsi]) && !pShockPlayer->HasTrait(PlayerObject(), kTraitPsionic)))
         {
            kind = 0;
            draw_border = TRUE;
         }
         else
         {
            kind = 1;
            draw_border = TRUE;
         }
      }
      else
      {
         if (!pShockPlayer->PsiLevel(obj, gPsiTrainData.m_level))
            kind = 0;
         else if (pShockPlayer->PsiPower(obj,(ePsiPowers)power))
            kind = 2;
         else if (pool < cost) // ((gPsiTrainData.m_level + 1 > basestats->m_stats[kStatPsi]) && !pShockPlayer->HasTrait(PlayerObject(), kTraitPsionic)))
            kind = 0;
         else
            kind = 1;
      }

      ShockDrawPsiIcon((ePsiPowers)power,dx,dy,kind);

      if (kind != 2)
      {
         sprintf(temp,"%d",cost);
         gr_font_string(gShockFont, temp, dx + 44, dy + 15);
      }

      if (!draw_border)
      {
         if (kind == 1)
         {
            drawpt.x = dx;
            drawpt.y = dy;
            DrawByHandle(gButtonHnd, drawpt);
         }
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

   if (draw_border)
   {
      drawpt.x = PSI_X1 + r.ul.x;
      drawpt.y = PSI_Y + r.ul.y;
      DrawByHandle(gBigButtonHnd, drawpt);
   }

   // draw the current stat description
   DrawMouseOverText(obj);

   LGadDrawBox(VB(&close_button),NULL);
   LGadDrawBox(VB(&undo_button),NULL);
}
//--------------------------------------------------------------------------------------
bool ShockBuyPsiHandleMouse(Point mpt)
{
   Rect r = ShockOverlayGetRect(kOverlayBuyPsi);
   ObjID obj;
   int cost,pool;
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
      gPsiTrainData.m_level = newlevel;
   }
   // click on arrows
   slot = FindSlot(mpt);
   if (slot == 1)
   {
      power = GetPsiPower(mpt);
      if (power == -1)
         return(TRUE);

      // do we have a high enough PSI to buy this power?
      sStatsDesc *basestats;
      if (!g_BaseStatsProperty->Get(obj, &basestats))
         return(TRUE);
      /*
      if ((gPsiTrainData.m_level + 1 > basestats->m_stats[kStatPsi]) && !pShockPlayer->HasTrait(PlayerObject(), kTraitPsionic))
         return(TRUE);
      */

      cost = pShockPlayer->PsiPowerCost(power);
      pool = pShockPlayer->GetPool(obj);
      // make sure we don't already have it
      if (!pShockPlayer->PsiPower(obj,(ePsiPowers)power)) // valid purchase
      {
         // make sure we have the appropriate tier discipline
         int tier;
         char temp[255];
         tier = pShockPlayer->FindPsiLevel((ePsiPowers)power);
         if (pShockPlayer->IsPsiTier((ePsiPowers)power) || pShockPlayer->PsiLevel(obj,tier-1))
         {
            if (pool >= cost)
            {
               // pay the price
               pShockPlayer->AddPool(obj, -1 * cost);

               // get the ability
               pShockPlayer->AddPsiPower(obj,(ePsiPowers)power);

               // recalc stats, since tier powers increase psi max
               pShockPlayer->RecalcData(obj);
            }
            else
            {
               ShockStringFetch(temp,sizeof(temp),"ErrorExpensive", "misc");
               ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
            }
         }
         else
         {
            ShockStringFetch(temp,sizeof(temp),"ErrorNoTier", "misc");
            ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
         }
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);
   uiDefer(DeferOverlayClose,(void *)kOverlayBuyPsi);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool undo_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);

   AutoAppIPtr(ShockPlayer);
   ObjID o = PlayerObject();
   pShockPlayer->SetPool(o,gUndoPool);
   
   g_PsiPowerProperty->Set(o, gUndoPowers);
   g_PsiPower2Property->Set(o, gUndoPowers2);

   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayBuyPsi);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   undo_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   undo_elem.draw_data = undo_bitmaps;
   undo_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&undo_button, LGadCurrentRoot(), undo_rect.ul.x + r.ul.x, undo_rect.ul.y + r.ul.y,
      RectWidth(&undo_rect), RectHeight(&undo_rect), &undo_elem, undo_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&undo_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockBuyPsiStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      BuildInterfaceButtons();

      ObjID o = PlayerObject();
      AutoAppIPtr(ShockPlayer);

      gUndoPool = pShockPlayer->GetPool(o);

      g_PsiPowerProperty->Get(o, &gUndoPowers);
      g_PsiPower2Property->Get(o, &gUndoPowers2);
   }
   else
   {
      DestroyInterfaceButtons();
   }
}
