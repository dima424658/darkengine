/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktechs.cpp,v 1.8 2000/02/19 13:26:24 toml Exp $

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
#include <shktechs.h>
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

// TECH SKILLS
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

void ShockTechSkillInit(int which)
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

   gStatBack = LoadPCX("technic");
   gArrowLitHnd = LoadPCX("skilstat");
   gArrowBoostHnd = LoadPCX("statboos");
   gNavDownHnd = LoadPCX("etech1");

   SetRightMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockTechSkillTerm(void)
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
   {{30,8},{178,34}}, // hacking
   {{30,35},{178,60}}, // repair
   {{30,61},{178,86}}, // modify
   {{30,87},{178,112}}, // mainteance
   {{30,113},{178,143}}, // research

   {{10,266},{176,291}}, // nav buttons

   {{17,144}, {17 + 33, 144 + 32}}, // hack soft
   {{99,144}, {99 + 33, 144 + 32}}, // modify soft
   {{58,144}, {58 + 33, 144 + 32}}, // repair soft
   {{140,144}, {140 + 33, 144 + 32}}, // research soft
};
#define NUM_STAT_RECTS (sizeof(stat_rects) / sizeof(Rect))
static Rect text_rect = {{15,180},{174,264}};
#define BDIM(x,y) {{x,y},{x+39,y+17}}
static Rect navdown_rect = BDIM(55,270);

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

   r = ShockOverlayGetRect(kOverlayTechSkill);

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   i = FindSlot(mpt);
   if (i != -1)
   {
      ShockStringFetch(temp,sizeof(temp),"tech","skilhelp",i);
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
void ShockTechSkillDraw(void)
{
   Rect r;
   sWeaponSkills *pWeapon;
   sTechSkills *pTech;
   int i,j;
   Point drawpt;
   int modval,baseval;
   int buy = 2;
   ObjID obj;
   AutoAppIPtr(ShockPlayer);

   obj = PlayerObject();
   
   // draw the background
   r = ShockOverlayGetRect(kOverlayTechSkill);
   DrawByHandle(gStatBack,r.ul);

   gr_set_fcolor(gShockTextColor);

   // draw in the arrows for current stat levels & potential
   // note that we are directly accessing the property here since we
   // want to know your BASE skills, not your post-effects info
   if (!g_BaseWeaponProperty->Get(obj, &pWeapon) || !g_BaseTechProperty->Get(obj, &pTech))
   {
      return;
   }

   for (i=kWeaponAnnelid + 1; i <= kWeaponAnnelid + kTechResearch + 1; i++)
   {
      if (i == kWeaponAnnelid + 1)
         drawpt.y = ARROW_Y + r.ul.y;

      if (i <= kWeaponAnnelid)
      {
         baseval = pWeapon->m_wpn[i];
         modval = pShockPlayer->GetWeaponSkill((eWeaponSkills)i);
      }
      else
      {
         baseval = pTech->m_tech[i - kWeaponAnnelid - 1];
         modval = pShockPlayer->GetTechSkill((eTechSkills)(i - kWeaponAnnelid - 1));
      }
      drawpt.x = ARROW_X + r.ul.x;
      if (modval == 0)
      {
         char temp[255];
         ShockStringFetch(temp,sizeof(temp),"SkillZero","misc");
         gr_font_string(gShockFont,temp,drawpt.x, drawpt.y);
      }
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
      drawpt.y = drawpt.y + ARROW_DY;
   }

   // draw in the software levels
   ObjID soft;
   int softlevel;
   int w,h;
   char temp[32];
   for (i = 0; i < 4; i++)
   {
      soft = pShockPlayer->GetEquip(PlayerObject(), (ePlayerEquip)(kEquipHack + i));
      if (soft == OBJ_NULL)
         continue;

      if (gPropSoftwareLevel->Get(soft, &softlevel))
      {
         sprintf(temp, "%d", softlevel);
         w = gr_font_string_width(gShockFont, temp);
         h = gr_font_string_height(gShockFont, temp);
         drawpt.x = r.ul.x + stat_rects[i + 6].ul.x + (RectWidth(&stat_rects[i + 6]) - w) / 2;
         drawpt.y = r.ul.y + stat_rects[i + 6].ul.y + (RectHeight(&stat_rects[i + 6]) - h) / 2;
         gr_font_string(gShockFont, temp, drawpt.x, drawpt.y); 
      }
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
bool ShockTechSkillHandleMouse(Point mpt)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      SchemaPlay((Label *)"subpanel_cl",NULL);
      uiDefer(DeferOverlayClose,(void *)kOverlayTechSkill);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayTechSkill);
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
void ShockTechSkillStateChange(int which)
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
bool ShockTechSkillCheckTransp(Point pt)
{
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
   return(FALSE);
}
