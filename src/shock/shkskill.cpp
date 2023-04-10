/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkskill.cpp,v 1.17 2000/02/19 13:26:15 toml Exp $

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
#include <shkskill.h>
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

static IRes *gStatBack = NULL;
static IRes *gArrowLitHnd = NULL;
static IRes *gArrowBoostHnd = NULL;
static IRes *gNavDownHnd = NULL;

static Rect full_rect = {{RMFD_X, RMFD_Y}, {RMFD_X + RMFD_W, RMFD_Y + RMFD_H}}; 
static Rect close_rect = {{5,8},{5 + 20, 8 + 21}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

//--------------------------------------------------------------------------------------
void ShockSkillsInit(int which)
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

   gStatBack = LoadPCX("combat");
   gArrowLitHnd = LoadPCX("skilstat");
   gArrowBoostHnd = LoadPCX("skilboos");
   gNavDownHnd = LoadPCX("ecmbt1");

   SetRightMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockSkillsTerm(void)
{
   SafeFreeHnd(&gStatBack);
   SafeFreeHnd(&gArrowLitHnd);
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

static Rect stat_rects[] = {
   {{30,8},{178,34}}, // standard
   {{30,35},{178,60}}, // energy
   {{30,61},{178,86}}, // heavy
   {{30,87},{178,112}}, // annelid

   {{10,266},{176,291}}, // nav buttons
};
#define NUM_STAT_RECTS (sizeof(stat_rects) / sizeof(Rect))
static Rect text_rect = {{15,180},{174,264}};
#define BDIM(x,y) {{x,y},{x+39,y+17}}
static Rect navdown_rect = BDIM(95,270);

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
static void DrawMouseOverText(void)
{
   Point mpt;
   int i;
   char temp[255];
   int dx,dy;
   Rect r;

   r = ShockOverlayGetRect(kOverlaySkills);

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   i = FindSlot(mpt);
   if (i != -1)
   {
      ShockStringFetch(temp,sizeof(temp),"weapon","skilhelp",i);
      gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rect));
      dx = r.ul.x + text_rect.ul.x; //+ (RectWidth(&text_rect) - w) / 2;
      dy = r.ul.y + text_rect.ul.y; //+ (RectHeight(&text_rect) - h) / 2;
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFont,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------
void ShockSkillsDraw(void)
{
   Rect r;
   sWeaponSkills *pWeapon;
   int i,j;
   Point drawpt;
   int modval,baseval;
   int buy = 2;
   ObjID obj;
   AutoAppIPtr(ShockPlayer);

   obj = PlayerObject();
   
   // draw the background
   r = ShockOverlayGetRect(kOverlaySkills);
   DrawByHandle(gStatBack,r.ul);

   // draw in the arrows for current stat levels & potential
   // note that we are directly accessing the property here since we
   // want to know your BASE skills, not your post-effects info
   if (!g_BaseWeaponProperty->Get(obj, &pWeapon))
   {
      return;
   }

   for (i=0; i <= kWeaponAnnelid; i++)
   {
      if (i == 0)
         drawpt.y = ARROW_Y + r.ul.y;
   
      baseval = pWeapon->m_wpn[i];
      modval = pShockPlayer->GetWeaponSkill((eWeaponSkills)i);

      if (modval == 0)
      {
         char temp[255];
         ShockStringFetch(temp,sizeof(temp),"SkillZero","misc");
         gr_font_string(gShockFont,temp,ARROW_X + r.ul.x, drawpt.y);
      }
      else
      {
         drawpt.x = ARROW_X + r.ul.x;
         for (j=0; j < MAX_STAT_VAL; j++)
         {
            if (j < baseval)
               DrawByHandle(gArrowLitHnd, drawpt);
            else if (j < modval)
               DrawByHandle(gArrowBoostHnd, drawpt);
            else
               break;
            drawpt.x = drawpt.x + ARROW_DX;
         }
      }
      drawpt.y = drawpt.y + ARROW_DY;
   }

   // draw the current stat description
   DrawMouseOverText();

   // draw in misc nav type buttons
   LGadDrawBox(VB(&close_button),NULL);
   ShockMFDNavButtonsDraw();
   // draw on top of the nav button our "down" state
   drawpt.x = navdown_rect.ul.x + r.ul.x;
   drawpt.y = navdown_rect.ul.y + r.ul.y;
   DrawByHandle(gNavDownHnd, drawpt);
}
//--------------------------------------------------------------------------------------
bool ShockSkillsHandleMouse(Point mpt)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      SchemaPlay((Label *)"subpanel_cl",NULL);
      uiDefer(DeferOverlayClose,(void *)kOverlaySkills);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlaySkills);
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
void ShockSkillsStateChange(int which)
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
bool ShockSkillsCheckTransp(Point pt)
{
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
   return(FALSE);
}


