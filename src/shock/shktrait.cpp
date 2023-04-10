/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktrait.cpp,v 1.21 2000/02/19 13:26:28 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>
#include <schema.h>
#include <scrnmode.h>
#include <scrptapi.h>

#include <playrobj.h>
#include <plyrmode.h>

#include <shkprop.h>
//#include <shkscrpt.h>
#include <shktrait.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkplprp.h>
#include <shkiftul.h>
#include <shktrcst.h>
#include <shkmfddm.h>
#include <shkinv.h>

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
static IRes *gNavDownHnd = NULL;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static Rect text_rect = {{15,214},{174,264}};
static Rect top_rect = {{15,25},{152,70}};
static Rect matrix_rect = {{15,76},{152,209}};

static BOOL gTraitBought;

#define TRAIT_X   15
#define TRAIT_Y   35
#define TRAIT_W   35
#define TRAIT_H   34

#define TITLE_X   17
#define TITLE_Y   14

#define MATRIX_X1 15
#define MATRIX_X2 153
#define MATRIX_Y  76

//--------------------------------------------------------------------------------------
void ShockTraitInit(int which)
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

   gStatBack = LoadPCX("Traits");
   gNavDownHnd = LoadPCX("eTrait1");

   /*
   sScrnMode smode;
   Rect use_rect;
   ScrnModeGet(&smode);
   use_rect.ul.x = smode.w - (640 - full_rect.ul.x);
   use_rect.ul.y = full_rect.ul.y;
   use_rect.lr.x = use_rect.ul.x + RectWidth(&full_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&full_rect);
   
   ShockOverlaySetRect(which, use_rect);
   */
   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockTraitTerm(void)
{
   SafeFreeHnd(&gStatBack);
   SafeFreeHnd(&gNavDownHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
static int FindTrait(ObjID obj, Point mpt)
{
   sTraitsDesc *pTrait;
   int which;

   if (!g_TraitsProperty->Get(obj, &pTrait))
      return(-1);

   // look in the top slot area
   if (RectTestPt(&top_rect, mpt))
   {
      which = (mpt.x - top_rect.ul.x) / TRAIT_W;
      return (pTrait->m_traits[which]);
   }

   // look through the matrix
   if (RectTestPt(&matrix_rect, mpt) && !gTraitBought)
   {
      which = (mpt.x - matrix_rect.ul.x) / TRAIT_W;
      which = which + ((mpt.y - matrix_rect.ul.y) / TRAIT_H) * 4;
      which = which + 1; // since we don't start with kTraitEmpty
      //mprintf("which = %d\n",which);
      if ((which >= kTraitMax) || (which < kTraitEmpty))
      {
         Warning(("FindTrait: computed invalid trait %d!\n",which));
         return(-1);
      }
      return(which);
   }

   return(-1);
}
//--------------------------------------------------------------------------------------
static void DrawMouseOverText(void)
{
   Point mpt;
   int i;
   char temp[255];
   int dx,dy;
   Rect r;

   r = ShockOverlayGetRect(kOverlayBuyTraits);

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   i = FindTrait(PlayerObject(), mpt);
   if (i != -1)
   {
      ShockStringFetch(temp,sizeof(temp),"Trait","Traits",i);
      gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rect));
      dx = r.ul.x + text_rect.ul.x; //+ (RectWidth(&text_rect) - w) / 2;
      dy = r.ul.y + text_rect.ul.y; //+ (RectHeight(&text_rect) - h) / 2;
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFont,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------
void ShockTraitDrawIcon(eTrait which, int dx, int dy)
{
   char temp[255];
   IRes *hnd;
   grs_bitmap *bmp;

   if ((which >= kTraitMax) || (which < kTraitEmpty))
      return;

   sprintf(temp,"trait%02d",which);
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
void ShockTraitDraw(void)
{
   Rect r;
   sTraitsDesc *pTrait;
   int dx,dy;
   int i;
   ObjID obj;
   AutoAppIPtr(ShockPlayer);

   obj = PlayerObject();

   // draw the background
   r = ShockOverlayGetRect(kOverlayBuyTraits);
   DrawByHandle(gStatBack,r.ul);

   // draw top icons (already bought)
   if (g_TraitsProperty->Get(obj, &pTrait))
   {
      dy = TRAIT_Y + r.ul.y;
      dx = TRAIT_X + r.ul.x;
      for (i=0; i < NUM_TRAIT_SLOTS; i++)
      {
         if (pTrait->m_traits[i] > 0)
         {
            ShockTraitDrawIcon((eTrait)pTrait->m_traits[i], dx, dy);
            dx = dx + TRAIT_W;
         }
      }
      if (!gTraitBought)
         ShockTraitDrawIcon(kTraitEmpty,dx,dy);
   }

   if (!gTraitBought)
   {
      // draw matrix
      dx = MATRIX_X1 + r.ul.x;
      dy = MATRIX_Y + r.ul.y;
      for (i=kTraitEmpty + 1; i <= kTraitMax; i++)
      {
         // skip any traits we already have
         if (!pShockPlayer->HasTrait(obj, (eTrait)i))
            ShockTraitDrawIcon((eTrait)i,dx,dy);

         // advance to next
         dx = dx + TRAIT_W;
         if (dx >= r.ul.x + MATRIX_X2)
         {
            dx = MATRIX_X1 + r.ul.x;
            dy = dy + TRAIT_H;
         }
      }
   }

   // draw in the "header"
   char temp[255];
   ShockStringFetch(temp,sizeof(temp),"TraitHeader","misc");
   gr_font_string(gShockFont,temp,TITLE_X + r.ul.x, TITLE_Y + r.ul.y);

   // draw the current stat description
   DrawMouseOverText();

   // draw in misc nav type buttons
   LGadDrawBox(VB(&close_button),NULL);
   //ShockMFDNavButtonsDraw();
}
//--------------------------------------------------------------------------------------
// it MUST be the case that you don't already have the trait when
// you call this function.
static void BuyTrait(ObjID obj, eTrait which)
{
   int i;
   sTraitsDesc *pTrait;
   AutoAppIPtr(ShockPlayer);

   if (!g_TraitsProperty->Get(obj,&pTrait))
      return;

   if (gTraitBought)
      return;

   gTraitBought = TRUE;
   // signal our script, so that the object knows not to be used again
   AutoAppIPtr(ScriptMan);
   sScrMsg msg(ShockOverlayGetObj(), "Used"); 
   // We need to mark the *local* copy not to be reused.
   msg.flags |= kSMF_MsgSendToProxy;
   pScriptMan->SendMessage(&msg); 

   for (i=0; i < NUM_TRAIT_SLOTS; i++)
   {
      if (pTrait->m_traits[i] == 0)
      {
         pTrait->m_traits[i] = which;
         g_TraitsProperty->Set(obj,pTrait);
         switch (which)
         {
         case kTraitAble:
            // get +3 pool points
            pShockPlayer->AddPool(obj,8);
            break;
         /*  
         case kTraitEgghead:
            // get +1 to all tech skills
            {
               sTechSkills skill, *pSkill;
               int i;
               if (!g_BaseTechProperty->Get(obj, &pSkill))
                  break;
               memcpy(&skill, pSkill, sizeof(sTechSkills));
               for (i=kTechHacking; i <= kTechResearch; i++)
               {
                  if (skill.m_tech[i] < MAX_SKILL_VAL)
                     skill.m_tech[i] = pSkill->m_tech[i] + 1;
               }
               g_BaseTechProperty->Set(obj, &skill);
            }
            break;
         */
         case kTraitSpeedy:
            AddSpeedScale("TraitSpeedy", 1.15, 1.0);
            break;
         case kTraitCybernetic:
            ShockInvRefresh();
            break;
         }
         // since this is always safe to call, and some traits
         // affect it...
         pShockPlayer->RecalcData(obj);
         return;
      }
   }
   Warning(("BuyTrait: Hey, no available trait slots?\n"));
}
//--------------------------------------------------------------------------------------
bool ShockTraitHandleMouse(Point mpt)
{
   Rect r = ShockOverlayGetRect(kOverlayBuyTraits);
   ObjID obj = PlayerObject();
   int which;
   sTraitsDesc *pTrait;
   AutoAppIPtr(ShockPlayer);

   if (shock_cursor_mode != SCM_NORMAL) 
      return(TRUE);

   if (!g_TraitsProperty->Get(obj, &pTrait))
      return(TRUE);

   if (RectTestPt(&matrix_rect,mpt))
   {
      which = FindTrait(obj,mpt);
      if (!pShockPlayer->HasTrait(obj,(eTrait)which))
      {
         // we don't already have it, so lets buy it!
         BuyTrait(obj,(eTrait)which);
         return(TRUE);
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);
   uiDefer(DeferOverlayClose,(void *)kOverlayBuyTraits);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayBuyTraits);
   //ShockMFDNavButtonsBuild(&r);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   //ShockMFDNavButtonsDestroy();

   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockTraitStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      BuildInterfaceButtons();
      gTraitBought = FALSE;
   }
   else
   {
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockTraitCheckTransp(Point pt)
{
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
   return(FALSE);
}
