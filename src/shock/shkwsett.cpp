/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkwsett.cpp,v 1.22 2000/02/19 13:26:36 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>
#include <lgd3d.h>

#include <resapi.h>

#include <objsys.h>
#include <osysbase.h>
#include <mprintf.h>
#include <inv_rend.h>
#include <gamestr.h>
#include <schema.h>

#include <scrnman.h>

#include <shkprop.h>
#include <shkwsett.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <gunapi.h>
#include <shkcurm.h>
#include <shkwsprp.h>
#include <shkmfddm.h>
#include <shkhplug.h>
#include <shkobjst.h>
#include <shkiftul.h>
#include <shkpgapi.h>
#include <shkmelee.h>
#include <gunprop.h>
#include <shkplcst.h>
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

static IRes *gSettingBack = NULL;
static IRes *gSettingSelect = NULL;

// API supports 3, but interface laid out for 2.  Easy to add a 3rd if need be
#define NUM_WEAPON_SETTINGS   2
static Rect select_rects[NUM_WEAPON_SETTINGS] = {
   {{11,150},{175,218}},
   {{11,220},{175,278}},
};
static Rect text_rects[NUM_WEAPON_SETTINGS] = {
   {{22,153},{165,212}},
   {{22,223},{165,272}},
};

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static Rect unload_rect = {{13,105},{13 + 142, 105 + 22}};

static LGadButton unload_button;
static DrawElement unload_elem;
static IRes *unload_handles[2];
static grs_bitmap *unload_bitmaps[4];

extern "C" BOOL g_lgd3d;

#define TEXT_X 24
#define TEXT_Y 133

//--------------------------------------------------------------------------------------
static invRendState *cur_inv_rend=NULL;
Rect *get_model_draw_rect(void);
void inv_show_model(void);

//--------------------------------------------------------------------------------------
void ShockSettingInit(int which)
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

   unload_handles[0] = LoadPCX("unload0"); 
   unload_handles[1] = LoadPCX("unload1"); 
   unload_bitmaps[0] = (grs_bitmap *) unload_handles[0]->Lock();
   unload_bitmaps[1] = (grs_bitmap *) unload_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      unload_bitmaps[i] = unload_bitmaps[0];
   }

   gSettingBack = LoadPCX("settings");
   gSettingSelect = LoadPCX("setsel");
   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockSettingTerm(void)
{
   SafeFreeHnd(&gSettingBack);
   SafeFreeHnd(&gSettingSelect);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   unload_handles[0]->Unlock();
   unload_handles[1]->Unlock();
   SafeFreeHnd(&unload_handles[0]);
   SafeFreeHnd(&unload_handles[1]);
}

//--------------------------------------------------------------------------------------
char *setting_propnames[2] = { PROP_SETTINGTEXT1_NAME, PROP_SETTINGTEXT2_NAME };
char *setting_headlines[2] = { PROP_SETTINGHEAD1_NAME, PROP_SETTINGHEAD2_NAME };

void ShockSettingDraw(void)
{
   Rect r;
   Point drawpt;
   int dx,dy;
   char temp[255],t2[255];
   cStr str;
   int usesetting;
   int i;
   ObjID mfd_obj;
 
   AutoAppIPtr(GameStrings);

   r = ShockOverlayGetRect(kOverlaySetting);
   mfd_obj = ShockOverlayGetObj();
   drawpt = r.ul;
   DrawByHandle(gSettingBack,drawpt);

   // draw the model itself
   inv_show_model();

   gr_set_fcolor(gShockTextColor);

   // draw in the shortname
   str = pGameStrings->FetchObjString(mfd_obj,PROP_OBJSHORTNAME_NAME);
   strcpy(temp,str);
   dx = r.ul.x + TEXT_X;
   dy = r.ul.y + TEXT_Y;
   gr_font_string(gShockFont,temp,dx,dy);

   ShockStringFetch(temp,sizeof(temp),"ModLevel","misc");
   dx = r.ul.x + TEXT_X;
   dy = r.ul.y + TEXT_Y - 50;
   sprintf(t2,temp,GunGetModification(mfd_obj));
   gr_font_string(gShockFont,t2,dx,dy);

   for (i=0; i < NUM_WEAPON_SETTINGS; i++)
   {
      char t2[255];
      // draw in the headliners
      str = pGameStrings->FetchObjString(mfd_obj,setting_headlines[i]);
      strcpy(t2,str);
      dx = r.ul.x + text_rects[i].ul.x;
      dy = r.ul.y + text_rects[i].ul.y;
//      gr_font_string(gShockFont,temp,dx,dy);

      // draw in the setting text
      str = pGameStrings->FetchObjString(mfd_obj,setting_propnames[i]);
      sprintf(temp,"%s: %s",t2,str);
      gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rects[i]));
      gr_font_string(gShockFont, temp, dx, dy);
   }

   // draw the selection overlay
   usesetting = GunGetSetting(mfd_obj);
   drawpt.x = r.ul.x + select_rects[usesetting].ul.x;
   drawpt.y = r.ul.y + select_rects[usesetting].ul.y;
   DrawByHandle(gSettingSelect, drawpt);

   int wpntype;
   g_pWeaponTypeProperty->Get(mfd_obj,&wpntype);
   if (wpntype != kWeaponEnergy)
      LGadDrawBox(VB(&unload_button),NULL);

   LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
static void ChangeSetting(int newval)
{
   SchemaPlay((Label *)"bset",NULL);
   GunSetSetting(ShockOverlayGetObj(), newval);
}
//--------------------------------------------------------------------------------------
bool ShockSettingHandleMouse(Point mpt)
{
   Rect r = ShockOverlayGetRect(kOverlaySetting);
   int i;

   if (shock_cursor_mode != SCM_NORMAL) 
      return(TRUE);

   for (i=0; i < NUM_WEAPON_SETTINGS; i++)
   {
      if (RectTestPt(&select_rects[i], mpt))
      {
         ChangeSetting(i);
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlaySetting);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool unload_cb(short action, void* data, LGadBox* vb)
{
   ObjID gunobj;
   //AutoAppIPtr(PlayerGun);
   gunobj = ShockOverlayGetObj(); // pPlayerGun->Get();

   // do nothing if we are an energy weapon
   // also do nothing if we are in a non-standard cursor mode
   int wpntype;
   g_pWeaponTypeProperty->Get(gunobj,&wpntype);
   if ((wpntype != kWeaponEnergy) && (shock_cursor_mode == SCM_NORMAL))
   {
      GunUnload(gunobj, LF_AUTOPLACE);
      ShockInvReset(); // do a full recompute
      SchemaPlay((Label *)"bammo",NULL);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlaySetting);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   unload_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   unload_elem.draw_data = unload_bitmaps;
   unload_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&unload_button, LGadCurrentRoot(), unload_rect.ul.x + r.ul.x, unload_rect.ul.y + r.ul.y,
      RectWidth(&unload_rect), RectHeight(&unload_rect), &unload_elem, unload_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&unload_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockSettingStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      BuildInterfaceButtons();
      // just got put up
      if (!cur_inv_rend)
      {
         cur_inv_rend=invRendBuildState(0,ShockOverlayGetObj(),get_model_draw_rect(), NULL);
         invRendUpdateState(cur_inv_rend,INVREND_ROTATE|INVREND_HARDWARE_IMMEDIATE|INVREND_SET,OBJ_NULL,NULL,NULL);
      }

      ObjID o;
      o = ShockOverlayGetObj();
      if (ObjGetObjState(o) == kObjStateBroken)
      {
         ShockHRMPlugSetMode(1, o);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }
      else 
      {
         int wpntype;
         g_pWeaponTypeProperty->Get(o,&wpntype);
         if (!IsMelee(o) && !(wpntype == kWeaponPsiAmp))
         {
            ShockHRMPlugSetMode(2, o);
            ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
         }
      }
   }
   else
   {
      DestroyInterfaceButtons();
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
      // just got taken down
      if (cur_inv_rend)
      {
         invRendFreeState(cur_inv_rend);
         cur_inv_rend=NULL;
      }
   }
}

//--------------------------------------------------------------------------------------
// and now, a bunch of stuff stolen and slightly modified from Dark's inv_hack.c
//--------------------------------------------------------------------------------------
#define OBJAREA_X 15
#define OBJAREA_Y 14
static Rect *get_model_draw_rect(void)
{
   static Rect draw_rect;

   draw_rect = ShockOverlayGetRect(kOverlaySetting);
   draw_rect.ul.x += OBJAREA_X;
   draw_rect.ul.y += OBJAREA_Y;
   draw_rect.lr.x = draw_rect.ul.x + 138;
   draw_rect.lr.y = draw_rect.ul.y + 109;
   return &draw_rect;
}
//--------------------------------------------------------------------------------------
static void inv_show_model(void)
{
   if (cur_inv_rend)
   {
      invRendUpdateState(cur_inv_rend,0,ShockOverlayGetObj(),get_model_draw_rect(),NULL);
      invRendDrawUpdate(cur_inv_rend);
   }
}
//--------------------------------------------------------------------------------------

sOverlayFunc OverlaySetting = { 
   ShockSettingDraw, // draw
   ShockSettingInit,// init
   ShockSettingTerm, // term
   ShockSettingHandleMouse, // mouse
   NULL, // dclick (really, use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   ShockSettingStateChange, // state
   NULL, // transparency
   0, // distance
   TRUE, // needmouse
   0, // alpha
   NULL, // update func
   TRUE, // check contains?
};
