/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkammov.cpp,v 1.41 2000/02/19 12:36:34 toml Exp $

#include <2d.h>

#include <res.h>
#include <guistyle.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>

#include <objtype.h>
#include <contain.h>
#include <playrobj.h>
#include <traitman.h>
#include <scrnmode.h>
#include <gamestr.h>
#include <command.h>
#include <schema.h>
#include <questapi.h>
#include <plyrmode.h>

#include <shkgame.h>
#include <shkutils.h>
#include <shkovrly.h>
#include <shkpgapi.h>
#include <gunapi.h>
#include <shkpsapi.h>
#include <shkpsiif.h>
#include <gunprop.h>
#include <shkinv.h>
#include <shkovcst.h>
#include <shkplcst.h>
#include <shkplayr.h>
#include <shkcurm.h>
#include <shkprop.h>
#include <shkobjst.h>
#include <shkincst.h>
#include <shkqbind.h>
#include <shkmelpr.h>
#include <shkifstr.h>

#include <shkammov.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

IRes *gHndAmmoBack = NULL;
IRes *gHndAmmoBack2 = NULL;
ObjID gPrevGun = OBJ_NULL;

static BOOL g_interface = FALSE;

static ePsiPowers gLastPower[5];

static Rect setting_rect = {{118,15},{118 + 66, 15 + 20}};
static LGadButton setting_button;
static DrawElement setting_elem;
static IRes *setting_handles[2];
static grs_bitmap *setting_bitmaps[4];

static Rect cycle_rect = {{186,15},{186+12, 15 + 41}};
static LGadButton cycle_button;
static DrawElement cycle_elem;
static IRes *cycle_handles[2];
static grs_bitmap *cycle_bitmaps[4];

static Rect reload_rect = {{118,37},{118+ 66, 37 + 20}};
static LGadButton reload_button;
static DrawElement reload_elem;
static IRes *reload_handles[2];
static grs_bitmap *reload_bitmaps[4];

#define NUM_WEAPSTATE_BITMAPS 11
static IRes *gWeapStateHnds[NUM_WEAPSTATE_BITMAPS];

//typedef enum ePsiLevelButton { kPsiButton1, kPsiButton2, kPsiButton3, kPsiButton4, kPsiButton5, };

#define NUM_IFACE_BUTTONS  6
extern Rect iface_rects[NUM_IFACE_BUTTONS];

#define NUM_PSILEVEL_BUTTONS 4
static Rect psilevel_rects[NUM_PSILEVEL_BUTTONS] = {
   // tier left / right
   {{118,15},{118 + 18, 15 + 18}}, 
   {{136,15},{136 + 18, 15 + 18}},

   // power left / right
   {{157,15},{157 + 12, 15 + 41}},
   {{237,15},{237 + 12, 15 + 41}},
};

static LGadButtonList psilevel_blist;
static LGadButtonListDesc psilevel_blistdesc;
static DrawElement psilevel_blist_elems[NUM_PSILEVEL_BUTTONS];
static IRes *psilevel_handles[NUM_PSILEVEL_BUTTONS][2];
static grs_bitmap *psilevel_bitmaps[NUM_PSILEVEL_BUTTONS][4];

static char *button_names[NUM_PSILEVEL_BUTTONS] = { "left","right","pleft","pright",};

//--------------------------------------------------------------------------------------
// Ammo indicator initializer
//--------------------------------------------------------------------------------------
#define AMMO_MODE_DX 166
#define AMMO_X 544
#define AMMO_Y 414
Rect ammo_rect = {{AMMO_X,AMMO_Y},{AMMO_X + 94,AMMO_Y+ 64}};
Rect ammotype_rect = {{200, 14}, {249,55}};
Rect ammotype_rect2 = {{177, 14}, {249,55}};
Rect psi_rect = {{177,16},{249,55}};

void ShockAmmoInit(int which)
{
   gHndAmmoBack = LoadPCX("ammoback");
   gHndAmmoBack2 = LoadPCX("ammofull");

   int i,n;
   char temp[255];

   for (i=0; i < 5; i++)
      gLastPower[i] = kPsiNone; // (ePsiPowers)(psi_levels[i] + 1);

   for (i=0; i < NUM_WEAPSTATE_BITMAPS; i++)
   {
      sprintf(temp,"wstate%d",i+1);
      gWeapStateHnds[i] = LoadPCX(temp);
   }

   setting_handles[0] = LoadPCX("ammo0"); 
   setting_handles[1] = LoadPCX("ammo1"); 
   setting_bitmaps[0] = (grs_bitmap *) setting_handles[0]->Lock();
   setting_bitmaps[1] = (grs_bitmap *) setting_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      setting_bitmaps[i] = setting_bitmaps[0];
   }

   cycle_handles[0] = LoadPCX("ammoarw0"); 
   cycle_handles[1] = LoadPCX("ammoarw1"); 
   cycle_bitmaps[0] = (grs_bitmap *) cycle_handles[0]->Lock();
   cycle_bitmaps[1] = (grs_bitmap *) cycle_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      cycle_bitmaps[i] = cycle_bitmaps[0];
   }

   reload_handles[0] = LoadPCX("ammo0"); 
   reload_handles[1] = LoadPCX("ammo1"); 
   reload_bitmaps[0] = (grs_bitmap *) reload_handles[0]->Lock();
   reload_bitmaps[1] = (grs_bitmap *) reload_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      reload_bitmaps[i] = reload_bitmaps[0];
   }

   for (n=0; n < NUM_PSILEVEL_BUTTONS; n++)
   {
      for (i=0; i < 2; i++)
      {
         sprintf(temp,"%s%d",button_names[n],i);
         psilevel_handles[n][i] = LoadPCX(temp); 
         psilevel_bitmaps[n][i] = (grs_bitmap *) psilevel_handles[n][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         psilevel_bitmaps[n][i] = psilevel_bitmaps[n][0];
      }
   }

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);
   use_rect.ul.x = smode.w - (640 - ammo_rect.ul.x) - AMMO_MODE_DX;
   use_rect.ul.y = smode.h - (480 - ammo_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&ammo_rect) + AMMO_MODE_DX;
   use_rect.lr.y = use_rect.ul.y + RectHeight(&ammo_rect);

   ShockOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
// Main Meters shutdown
//--------------------------------------------------------------------------------------
void ShockAmmoTerm(void)
{
   SafeFreeHnd(&gHndAmmoBack);
   SafeFreeHnd(&gHndAmmoBack2);
   
   cycle_handles[0]->Unlock();
   cycle_handles[1]->Unlock();
   SafeFreeHnd(&cycle_handles[0]);
   SafeFreeHnd(&cycle_handles[1]);

   setting_handles[0]->Unlock();
   setting_handles[1]->Unlock();
   SafeFreeHnd(&setting_handles[0]);
   SafeFreeHnd(&setting_handles[1]);

   reload_handles[0]->Unlock();
   reload_handles[1]->Unlock();
   SafeFreeHnd(&reload_handles[0]);
   SafeFreeHnd(&reload_handles[1]);

   int n;

   for (n=0; n < NUM_WEAPSTATE_BITMAPS; n++)
   {
      SafeFreeHnd(&gWeapStateHnds[n]);
   }

   for (n=0; n < NUM_PSILEVEL_BUTTONS; n++)
   {
      psilevel_handles[n][0]->Unlock();
      psilevel_handles[n][1]->Unlock();
      SafeFreeHnd(&psilevel_handles[n][0]);
      SafeFreeHnd(&psilevel_handles[n][1]);
   }
}

//--------------------------------------------------------------------------------------
// Draw main Ammo
//--------------------------------------------------------------------------------------
extern char *setting_headlines[2];

// yow, this is getting really messy
void ShockAmmoDraw(void)
{
   char temp[255];
   ObjID ammotype;
   Rect r;
   int wpntype;
   cStr str;
   Rect use_rect;
   eObjState state;
   int mode, dx, dy,w, h;

   AutoAppIPtr(PlayerGun);
   AutoAppIPtr(GameStrings);
   AutoAppIPtr(PlayerPsi);
   AutoAppIPtr(ShockPlayer);

   r = ShockOverlayGetRect(kOverlayAmmo);

   Point pt;
   pt.x = r.ul.x;
   pt.y = r.ul.y;
   if (!shock_mouse)
   {
      pt.x = pt.x + AMMO_MODE_DX;
      DrawByHandle(gHndAmmoBack, pt);
   }
   else
      DrawByHandle(gHndAmmoBack2, pt);

   if (gPrevGun == OBJ_NULL)
      return;

   // find out what kind of weapon it is
   g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

   // if psi amp, draw currently selected psi power
   if (wpntype == kWeaponPsiAmp)
   {
      ePsiPowers power = pPlayerPsi->GetSelectedPower();
      if (power != kPsiNone)
      {
         if (shock_mouse)
         {
            dx = r.ul.x + 170;
            dy = r.ul.y + 15;
         }
         else
         {
            dx = r.ul.x + psi_rect.ul.x + ((RectWidth(&psi_rect) - 66) / 2);
            dy = r.ul.y + psi_rect.ul.y; //  + ((RectHeight(&psi_rect) - 30) / 2);
         }
         ShockDrawPsiIcon(power, dx, dy, 2);

         int bind = ShockBindVal(power);
         if (bind > 0)
         {
            char temp[32];
            sprintf(temp,"F%d",bind);
            gr_font_string(gShockFont,temp,dx,dy);
         }

         ShockStringFetch(temp,sizeof(temp),"PsiShort","psihelp",power);
         w = gr_font_string_width(gShockFont,temp);
         dx = dx + ((66 - w) / 2);
         dy = dy + 29;
         gr_font_string(gShockFont, temp, dx, dy);
      }

      if (shock_mouse)
      {
         extern grs_font *gKeypadFont;

         LGadDrawBox(VB(&psilevel_blist),NULL);

         ePsiPowers currpower;
         int currlevel;
         currpower = pPlayerPsi->GetSelectedPower();
         if (currpower != kPsiNone)
         {
            currlevel = ePsiPowers(int(pShockPlayer->FindPsiLevel(currpower)) - 1);
            sprintf(temp,"%d", currlevel + 1);
            w = gr_font_string_width(gKeypadFont, temp);
            dx = r.ul.x + 118 + ((40 - w) / 2);
            dy = r.ul.y + 34;
            gr_set_fcolor(gShockTextColor);
            gr_font_string(gKeypadFont, temp, dx, dy);
         }
      }
   }
   else
   {
      if (g_pMeleeTypeProperty->IsRelevant(gPrevGun))
      {
         // we need some sort of melee-ish icon here
      }
      else
      {
         // otherwise, draw the current ammo 
         ammotype = GetProjectile(gPrevGun);
         // does this want to become some sort of icon representation 
         // rather than the literal objicon?

         if (shock_mouse)
            RectOffsettedRect(&ammotype_rect, r.ul, &use_rect);
         else
            RectOffsettedRect(&ammotype_rect2, r.ul, &use_rect);

         dx = use_rect.ul.x + (RectWidth(&use_rect) - INV_ICON_WIDTH) / 2;
         dy = use_rect.ul.y + (RectHeight(&use_rect) - INV_ICON_HEIGHT) / 2;
         ShockInvObjDraw(ammotype,dx,dy);

         str = pGameStrings->FetchObjString(ammotype,PROP_OBJSHORTNAME_NAME);
         strcpy(temp,str);
         w = gr_font_string_width(gShockFont, temp);
         h = gr_font_string_height(gShockFont, temp);
         dx = use_rect.ul.x + (RectWidth(&use_rect) - w) / 2;
         dy = use_rect.lr.y - h;
         gr_font_string(gShockFont, temp, dx, dy);

         dx = use_rect.ul.x + 2;
         dy = use_rect.ul.y + 2;
         sprintf(temp,"%d",GunGetAmmoCount(gPrevGun));
         gr_font_string(gShockFont, temp, dx, dy);

         Point pt;
         int n;
         state = ObjGetObjState(gPrevGun);
         if (state != kObjStateNormal)
            n = 10;  // use last one for invalid
         else
            n = ((100 - int(GunGetCondition(gPrevGun)) - 1) / 10);
         if (n < 0) 
            n = 0;
         if (n >= 10 ) 
            n = 9;
         pt.x = r.ul.x + 236;
         pt.y = r.ul.y + 15;
         DrawByHandle(gWeapStateHnds[n],pt);

         /*
         state = ObjGetObjState(gPrevGun);
         if (state != kObjStateNormal)
         {
            ShockStringFetch(temp, sizeof(temp), "ObjState", "misc", state);
         }
         else
         {
            char cond[255];
            GunGetConditionString(gPrevGun,cond,sizeof(cond));
            //(int)GunGetCondition(gPrevGun));
            sprintf(temp,"(%s%)",cond);
         }
         w = gr_font_string_width(gShockFont,temp);
         dx = use_rect.ul.x + ((RectWidth(&use_rect) - w) / 2);
         dy = use_rect.lr.y - 13;
         gr_font_string(gShockFont, temp, dx, dy);
         */

         if (shock_mouse)
         {
            LGadDrawBox(VB(&setting_button),NULL);

            if (wpntype != kWeaponEnergy)
            {
               LGadDrawBox(VB(&cycle_button),NULL);
               LGadDrawBox(VB(&reload_button),NULL);
            }
   
            mode = GunGetSetting(gPrevGun);
            str = pGameStrings->FetchObjString(gPrevGun,setting_headlines[mode]);
            strcpy(temp,str);
            w = gr_font_string_width(gShockFont,temp);
            h = gr_font_string_height(gShockFont,temp);
            dx = r.ul.x + setting_rect.ul.x + (RectWidth(&setting_rect) - w) / 2;
            dy = r.ul.y + setting_rect.ul.y +(RectHeight(&setting_rect) - h) / 2;
            gr_font_string(gShockFont, temp, dx, dy);

            if (wpntype != kWeaponEnergy)
            {
               ShockStringFetch(temp,sizeof(temp),"Reload","misc");
               w = gr_font_string_width(gShockFont,temp);
               h = gr_font_string_height(gShockFont,temp);
               dx = r.ul.x + reload_rect.ul.x + (RectWidth(&reload_rect) - w) / 2;
               dy = r.ul.y + reload_rect.ul.y +(RectHeight(&reload_rect) - h) / 2;
               gr_font_string(gShockFont, temp, dx, dy);
            }

            // mouseover help
            Point mpt;
            mouse_get_xy(&mpt.x,&mpt.y);
            mpt.x = mpt.x - r.ul.x;
            mpt.y = mpt.y - r.ul.y;
            if (RectTestPt(&cycle_rect,mpt) && (wpntype != kWeaponEnergy))
            {
               ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpAmmoCycle", "misc");
            }
            else if (RectTestPt(&setting_rect,mpt))
            {
               ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpSettings", "misc");
            }
            else if (RectTestPt(&reload_rect,mpt) && (wpntype != kWeaponEnergy))
            {
               ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpReload", "misc");
            }
         }
      }
   }

}

//--------------------------------------------------------------------------------------
// Basic Meters handling.  Also covers the inventory popup by virtue of
// calling the InvFindObjSlot which covers both quick slots & real slots
//--------------------------------------------------------------------------------------
bool ShockAmmoHandleMouse(Point pt)
{
   int wpntype;

   if (shock_cursor_mode == SCM_DRAGOBJ)
   {
      ShockInvAddObj(PlayerObject(),drag_obj);
      return(TRUE);
   }

   if (gPrevGun == OBJ_NULL)
      return(TRUE);

   g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

   if (wpntype == kWeaponPsiAmp)
   {
      int i;
      // skip out if already being handled by gadgets
      for (i=0; i < 5; i++)
         if (RectTestPt(&psilevel_rects[i],pt))
            return(TRUE);

      if (pt.x >= psilevel_rects[0].ul.x)
         CommandExecute("select_psipower");
   }
   else
   {
      /*
      if (!RectTestPt(&cycle_rect,pt) && !RectTestPt(&setting_rect,pt) && (pt.x >= cycle_rect.ul.x))
      {
         CommandExecute("reload_gun");
      }
      */
   }

   return(TRUE);
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockAmmoCheckTransp(Point p)
{
   // hm, perhaps this wants to just always return false so that 
   // we don't deal with the problems of the interlacing?
   return(FALSE);
   /*
   bool retval = FALSE;
   int pix = HandleGetPix(gHndAmmoBack,p);
   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
   */
}

//--------------------------------------------------------------------------------------
void ShockAmmoReload(void)
{
   sContainIter *scip;
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(PlayerGun);
   ObjID obj = PlayerObject();

   // no reloading if dead
   if (GetPlayerMode() == kPM_Dead)
      return;

   // no reloading if no interface
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
      return;

   // no reloading if a melee weapon
   if (pPlayerGun->Get() == OBJ_NULL)
      return;

   // no reloading if an energy weapon
   int wpntype;
   g_pWeaponTypeProperty->Get(pPlayerGun->Get(),&wpntype);
   if (wpntype == kWeaponEnergy)
      return;

   // if we have a psi amp, then do something completely different
   // scan through all the items in the player's inventory
   scip = pContainSys->IterStart(obj); // ,0,SHOCKCONTAIN_PDOLLBASE - 1);
   while (!scip->finished)
   {
      if (pPlayerGun->MatchesCurrentAmmo(scip->containee) && !IsPlayerGun(scip->containee))
      {
         GunLoad(pPlayerGun->Get(), scip->containee, LF_AUTOPLACE);
         pContainSys->IterEnd(scip);
         ShockInvReset(); // do a full recompute
         return;
      }
      pContainSys->IterNext(scip);
   }
   // nope, didn't find it...
   pContainSys->IterEnd(scip);

   // give some feedback
   char temp[255];
   ShockStringFetch(temp,sizeof(temp),"CantReload","misc");
   ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
}
//--------------------------------------------------------------------------------------
// find the next ammo type in inventory, and load it up
void ShockAmmoChangeTypes(void)
{
   ObjID obj, curgun, origproj, nextproj, arch;
   sContainIter *scip;
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(PlayerGun);
   AutoAppIPtr(TraitManager);

   obj = PlayerObject();

   // no reloading if dead
   if (GetPlayerMode() == kPM_Dead)
      return;

   // scan through all the items in the player's inventory
   if ((curgun = pPlayerGun->Get()) == OBJ_NULL)
      return;

   // find out what kind of weapon it is
   int wpntype;
   g_pWeaponTypeProperty->Get(curgun,&wpntype);

   // if psi amp, do something entirely different
   if (wpntype == kWeaponPsiAmp)
   {
      CommandExecute("select_psipower");
      return;
   }

   origproj = GetProjectile(curgun);
   nextproj = GetNextProjectile(curgun, origproj);
   while (nextproj != origproj)
   {
      scip = pContainSys->IterStart(obj); // ,0,SHOCKCONTAIN_PDOLLBASE - 1);
      while (!scip->finished)
      {
         arch = GetProjectileFromClip(scip->containee, curgun);
         if ((arch == nextproj) && !IsPlayerGun(scip->containee))
         {
            GunLoad(pPlayerGun->Get(), scip->containee, LF_JUGGLE);
            pContainSys->IterEnd(scip);
            ShockInvReset(); // do a full recompute
            return;
         }
         pContainSys->IterNext(scip);
      }
      // nope, didn't find it...
      pContainSys->IterEnd(scip);
      // go to the next in line
      // if we are cycling around, that will bump us out of the loop
      nextproj = GetNextProjectile(curgun, nextproj);
   }
}
//--------------------------------------------------------------------------------------
static bool psilevel_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   ePsiPowers currpower, newpower;
   int newlevel, currlevel;
   BOOL keepgoing = TRUE;
   int count = 0;
   int n, delta;

   AutoAppIPtr(PlayerPsi);
   AutoAppIPtr(ShockPlayer);

   currpower = pPlayerPsi->GetSelectedPower();
   currlevel = pShockPlayer->FindPsiLevel(currpower) - 1;
   switch(button)
   {
   case 0: // tier left
      delta = -1;
      // fallthrough
   case 1: // tier right
      if (button == 1)
         delta = 1;

      newlevel = currlevel;
      while (keepgoing)
      {
         newlevel = newlevel + delta;
         if (newlevel < 0)
            newlevel = 4;
         if (newlevel > 4)
            newlevel = 0;
         if (pShockPlayer->PsiLevel(PlayerObject(),newlevel))
         {
            for (n = psi_levels[newlevel]; n < psi_levels[newlevel + 1]; n++)
            {
               if (pShockPlayer->PsiPower(PlayerObject(),(ePsiPowers)n))
               {
                  keepgoing = FALSE;
                  break;
               }
            }
         }
         count++;
         if (count > 5)
         {
            keepgoing = FALSE;
            newlevel = currlevel;
         }
      }
      if (newlevel != currlevel) 
         ShockAmmoPsiLevel(newlevel);
      break;

   case 2: // power left
      delta = -1;
      // fallthrough
   case 3: // power right
      if (button == 3)
         delta = 1;

      newpower = currpower;
      while (keepgoing)
      {
         count++;
         newpower = ePsiPowers(int(newpower) + delta);
         if (pShockPlayer->IsPsiTier(newpower))
         {
            if (delta == -1)
               newpower = ePsiPowers(int(psi_levels[currlevel + 1]) - 1);
            else
               newpower = ePsiPowers(int(psi_levels[currlevel]) + 1);
         }

         if (pShockPlayer->PsiPower(PlayerObject(),newpower))
            keepgoing = FALSE;
         if (count > 8)
         {
            keepgoing = FALSE;
            newpower = kPsiNone;
         }
      }
      gLastPower[currlevel] = newpower;
      pPlayerPsi->Select(newpower);
      ShockPsiRefocus(newpower);
      break;
   }
   return(TRUE);
}

//--------------------------------------------------------------------------------------
void ShockAmmoPsiLevel(int button)
{
   ePsiPowers newpower, currpower;
   int p;

   AutoAppIPtr(PlayerPsi);
   AutoAppIPtr(ShockPlayer);

   // first, find out what power category our current power is in
   currpower = pPlayerPsi->GetSelectedPower();
   for (p = 0; p < 5; p++)
   {
      if (currpower < psi_levels[p+1])
      {
         break;
      }
   }

   // if different, just switch
   if ((p != button) && (gLastPower[button] != kPsiNone))
      newpower = gLastPower[button];
   else
   {
      ePsiPowers startpower;
      if (gLastPower[button] == kPsiNone)
      {
         // if we have a currently selected power of that
         // type, then stuff it with that instead
         if ((button + 1) == pShockPlayer->FindPsiLevel(currpower))
            gLastPower[button] = currpower;
         else
            gLastPower[button] = (ePsiPowers)(psi_levels[button]);
      }

      // otherwise, cycle
      BOOL go = TRUE;
      int count = 0;
      startpower = gLastPower[button];
      // count is just in case....
      while (go && (count < 22))
      {
         count++;
         newpower = (ePsiPowers)(gLastPower[button] + 1);

         // if we hit the next power category, loop around
         if (newpower == psi_levels[button + 1])
            newpower = (ePsiPowers)(psi_levels[button]);
         // if we have wrapped around, stop
         if (newpower == startpower)
         {
            go = FALSE;
            break;
         }
         // if the power is valid, stop
         if ((newpower != psi_levels[button]) && pShockPlayer->PsiPower(PlayerObject(),newpower))
         {
            go = FALSE;
            break;
         }
         gLastPower[button] = newpower;
      }
      if (newpower == psi_levels[button])
         newpower = kPsiNone;
      gLastPower[button] = newpower;
   }

   pPlayerPsi->Select(newpower);
   ShockPsiRefocus(newpower);
}
//--------------------------------------------------------------------------------------
static bool cycle_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   AutoAppIPtr(PlayerGun);
   ObjID gun = pPlayerGun->Get();
   int wpntype;
   g_pWeaponTypeProperty->Get(gun,&wpntype);
   if (wpntype == kWeaponEnergy)
      return(FALSE);

   SchemaPlay((Label *)"bammo",NULL);
   CommandExecute("cycle_ammo");
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool setting_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);
   SchemaPlay((Label *)"bset",NULL);
   CommandExecute("wpn_setting_toggle");
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool reload_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   AutoAppIPtr(PlayerGun);
   ObjID gun = pPlayerGun->Get();
   int wpntype;
   g_pWeaponTypeProperty->Get(gun,&wpntype);
   if (wpntype == kWeaponEnergy)
      return(FALSE);

   SchemaPlay((Label *)"bammo",NULL);
   CommandExecute("reload_gun");
   return(TRUE);
}
//--------------------------------------------------------------------------------------

static void BuildInterfaceButtons(void)
{
   int wpntype;

   g_interface = TRUE;

   AutoAppIPtr(PlayerGun);
   gPrevGun = pPlayerGun->Get();
   g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

   Rect r = ShockOverlayGetRect(kOverlayAmmo);

   if (gPrevGun == OBJ_NULL)
      return;

   if (wpntype == kWeaponPsiAmp)
   {
      int n;
      static Rect use_rects[NUM_PSILEVEL_BUTTONS];

      for (n=0; n < NUM_PSILEVEL_BUTTONS; n++)
      {
         DrawElement *elem = &psilevel_blist_elems[n];
         elem->draw_type = DRAWTYPE_BITMAPOFFSET;
         elem->draw_data = psilevel_bitmaps[n];
         elem->draw_data2 = (void *)4; // should be 2 but hackery required
         use_rects[n].ul.x = psilevel_rects[n].ul.x + r.ul.x;
         use_rects[n].ul.y = psilevel_rects[n].ul.y + r.ul.y;
         use_rects[n].lr.x = psilevel_rects[n].lr.x + r.ul.x;
         use_rects[n].lr.y = psilevel_rects[n].lr.y + r.ul.y;

      }
   
      psilevel_blistdesc.num_buttons = NUM_PSILEVEL_BUTTONS;
      psilevel_blistdesc.button_rects = use_rects;
      psilevel_blistdesc.button_elems = psilevel_blist_elems;
      psilevel_blistdesc.cb = psilevel_cb;

      LGadCreateButtonListDesc(&psilevel_blist, LGadCurrentRoot(), &psilevel_blistdesc);
   }
   else
   {
      cycle_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
      cycle_elem.draw_data = cycle_bitmaps;
      cycle_elem.draw_data2 = (void *)4; // should be 2 but hackery required

      LGadCreateButtonArgs(&cycle_button, LGadCurrentRoot(), cycle_rect.ul.x + r.ul.x, 
         cycle_rect.ul.y + r.ul.y,
         RectWidth(&cycle_rect), RectHeight(&cycle_rect), &cycle_elem, cycle_cb, 0);

      setting_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
      setting_elem.draw_data = setting_bitmaps;
      setting_elem.draw_data2 = (void *)4; // should be 2 but hackery required

      LGadCreateButtonArgs(&setting_button, LGadCurrentRoot(), setting_rect.ul.x + r.ul.x, 
         setting_rect.ul.y + r.ul.y,
         RectWidth(&setting_rect), RectHeight(&setting_rect), &setting_elem, setting_cb, 0);

      reload_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
      reload_elem.draw_data = reload_bitmaps;
      reload_elem.draw_data2 = (void *)4; // should be 2 but hackery required

      LGadCreateButtonArgs(&reload_button, LGadCurrentRoot(), reload_rect.ul.x + r.ul.x, 
         reload_rect.ul.y + r.ul.y,
         RectWidth(&reload_rect), RectHeight(&reload_rect), &reload_elem, reload_cb, 0);
   }
}
//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   int wpntype;
   if (gPrevGun == OBJ_NULL)
      return;

   if (g_interface == FALSE)
      return;

   g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

   if (wpntype == kWeaponPsiAmp)
   {
      LGadDestroyBox(VB(&psilevel_blist),FALSE);
   }
   else
   {
      LGadDestroyBox(VB(&cycle_button),FALSE);
      LGadDestroyBox(VB(&setting_button),FALSE);
      LGadDestroyBox(VB(&reload_button),FALSE);
   }
   gPrevGun = OBJ_NULL;
   g_interface = FALSE;
}
//--------------------------------------------------------------------------------------
void ShockAmmoRefreshButtons(void)
{
   if (ShockOverlayCheck(kOverlayAmmo))
   {
      DestroyInterfaceButtons();
      BuildInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
void ShockAmmoStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();
   }
   else
   {
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
