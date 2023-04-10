/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmeter.cpp,v 1.24 2000/02/19 13:25:40 toml Exp $

#include <2d.h>
#include <appagg.h>

#include <res.h>
#include <guistyle.h>

#include <resapi.h>
#include <mprintf.h>
#include <scrnmode.h>

#include <playrobj.h>
#include <objhp.h>
#include <plyrmode.h>

#include <netman.h>    // for IsNetworkGame()

#include <shkgame.h>
#include <shkpsapi.h>
#include <shkutils.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkcurm.h>
#include <shkinv.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shklooko.h>
#include <shkifstr.h>

#include <shkmeter.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gHndPsi = NULL;
IRes *gHndHealth = NULL;
IRes *gHndBio = NULL;
IRes *gHndBioFull = NULL;

//--------------------------------------------------------------------------------------
// HP / Psi Meters initializer
//--------------------------------------------------------------------------------------
#define BIO_X  2
#define BIO_Y  414
Rect meters_rect = {{BIO_X,BIO_Y},{BIO_X + 260,BIO_Y+ 64}}; // 128

void ShockMetersInit(int which)
{
   gHndPsi = LoadPCX("psibar");
   gHndHealth = LoadPCX("hpbar");
   gHndBio = LoadPCX("bio");
   gHndBioFull = LoadPCX("biofull");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);
   use_rect.ul.x = meters_rect.ul.x;
   use_rect.ul.y = smode.h - (480 - meters_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&meters_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&meters_rect);

   ShockOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
// Main Meters shutdown
//--------------------------------------------------------------------------------------
void ShockMetersTerm(void)
{
   SafeFreeHnd(&gHndPsi);
   SafeFreeHnd(&gHndHealth);
   SafeFreeHnd(&gHndBio);
   SafeFreeHnd(&gHndBioFull);
}

//--------------------------------------------------------------------------------------
// Draw health and Psionics meters.  
//--------------------------------------------------------------------------------------
#define PSI_X  8
#define PSI_Y  41
#define PSITEXT_X 92
#define PSITEXT_Y 41

#define HP_X   8
#define HP_Y   18
#define HPTEXT_X 92
#define HPTEXT_Y 18

static void DrawMeters()
{
   int psi, psimax;
   int hp, hpmax;
   int clippix;
   grs_bitmap *bm;
   char temp[255];
   Rect r = ShockOverlayGetRect(kOverlayMeters);

   // set initial vals in case we don't have relevant properties
   hp = hpmax = psi = psimax = 0;

   strcpy(temp,"");

   if ((gHndPsi== NULL) || (gHndHealth == NULL))
   {
      //WARNING(("Could not get meter art!\n"));
      return;
   }

   bm = (grs_bitmap *) gHndPsi->Lock();

   // Determine the percentages.
   AutoAppIPtr(PlayerPsi);
   psi = pPlayerPsi->GetPoints();
   psimax = pPlayerPsi->GetMaxPoints();

   ObjID po = PlayerObject();
   ObjGetHitPoints(po, &hp);
   if (hp < 0)
      hp = 0;
   if (GetPlayerMode() == kPM_Dead)
      hp = 0;
   ObjGetMaxHitPoints(po, &hpmax);

   grs_clip saveClip = grd_gc.clip;                 // Clip it good!

   if ((psi == 0) || (psimax == 0))
      clippix = 0;
   else
      clippix = (bm->w * psi / psimax);
   if (clippix < 0)
      clippix = 0;

   gr_safe_set_cliprect(PSI_X + r.ul.x, PSI_Y + r.ul.y,PSI_X + r.ul.x + clippix,PSI_Y + r.ul.y + bm->h);
   gr_bitmap(bm, PSI_X + r.ul.x , PSI_Y + r.ul.y);
   gHndPsi->Unlock();

   grd_gc.clip = saveClip;
   sprintf(temp,"%d", psi); //  / %g",psi,psimax);
   gr_set_fcolor(gShockTextColor);
   gr_font_string(gShockFont,temp, PSITEXT_X + r.ul.x, PSITEXT_Y + r.ul.y);

   bm = (grs_bitmap *) gHndHealth->Lock();

   if ((hp == 0) || (hpmax == 0))
      clippix = 0;
   else
      clippix = (bm->w * hp / hpmax);
   if (clippix < 0)
      clippix = 0;

   gr_safe_set_cliprect(HP_X + r.ul.x,HP_Y + r.ul.y,HP_X + r.ul.x + clippix,HP_Y + r.ul.y + bm->h);
   gr_bitmap(bm, HP_X + r.ul.x, HP_Y + r.ul.y);
   gHndHealth->Unlock();

   grd_gc.clip = saveClip;
   sprintf(temp,"%d", hp); //  / %d",hp,hpmax); 
   gr_font_string(gShockFont,temp, HPTEXT_X + r.ul.x, HPTEXT_Y + r.ul.y);
}

//--------------------------------------------------------------------------------------
// Draw main Meters
//--------------------------------------------------------------------------------------
void ShockMetersDraw(void)
{
   grs_bitmap *bm;
   IRes *useres;

   if (!shock_mouse)
      useres = gHndBio;
   else
      useres = gHndBioFull;
   bm = (grs_bitmap *)useres->Lock();
   Rect r = ShockOverlayGetRect(kOverlayMeters);
   gr_bitmap(bm, r.ul.x, r.ul.y);
   useres->Unlock();

   Point mpt;
   mouse_get_xy(&mpt.x,&mpt.y);
   if (RectTestPt(&r,mpt))
   {
      if (mpt.x - r.ul.x  < PSITEXT_X + 23)
      {
         if (mpt.y - r.ul.y < PSI_Y)
            ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpHP","misc");
         else
            ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpPP","misc");
      }
   }
   

   DrawMeters();
}

//--------------------------------------------------------------------------------------
// Basic Meters handling.  Also covers the inventory popup by virtue of
// calling the InvFindObjSlot which covers both quick slots & real slots
//--------------------------------------------------------------------------------------
bool ShockMetersHandleMouse(Point pt)
{
   bool retval = TRUE;

   if (shock_cursor_mode == SCM_DRAGOBJ)
      ShockInvAddObj(PlayerObject(),drag_obj);

   AutoAppIPtr(ShockPlayer);

   if (shock_cursor_mode == SCM_LOOK)
   {
      ObjID fakeobj = OBJ_NULL;
      extern Rect fake_rects[4];
      Point usept;
      Rect r = ShockOverlayGetRect(kOverlayTicker);
      Rect r2 = ShockOverlayGetRect(kOverlayMeters);

      usept.x = pt.x + r2.ul.x;
      usept.y = pt.y + r2.ul.y;

      // okay, we have to fixup the fake_rects for high resolution positioning, thus the offset
      Rect userect;
      Point offset;
      offset.x = 0;
      offset.y = r.ul.y;

      // query nanites?
      RectOffsettedRect(&fake_rects[0], offset, &userect);
      if (RectTestPt(&userect,usept))
      {
         fakeobj = pShockPlayer->GetEquip(PlayerObject(), kEquipFakeNanites);
      }

      // query cookies?
      RectOffsettedRect(&fake_rects[1], offset, &userect);
      if (RectTestPt(&userect,usept))
      {
         fakeobj = pShockPlayer->GetEquip(PlayerObject(), kEquipFakeCookies);
      }

      if (fakeobj != OBJ_NULL)
      {
         ShockLookPopup(fakeobj);
         ClearCursor();
      }
   }

   return(retval);
}
//--------------------------------------------------------------------------------------
// Handle drag/drop. This is mainly intended so we can split nanites.
// It only works in multiplayer, since it's kinda pointless in SP.
extern void ShockSplitStack(ObjID o, BOOL leaveEmpty);
bool ShockMetersDragDrop(Point pt, BOOL start)
{
   AutoAppIPtr(NetManager);
   if (!pNetManager->IsNetworkGame())
      return(TRUE);

   // For now, this only deals with the case where the player wants to
   // split his nanites or cookies
   if ((shock_cursor_mode == SCM_SPLIT) && start)
   {
      AutoAppIPtr(ShockPlayer);
      extern Rect fake_rects[4];
      Point usept;
      Rect r = ShockOverlayGetRect(kOverlayTicker);
      Rect r2 = ShockOverlayGetRect(kOverlayMeters);

      usept.x = pt.x + r2.ul.x;
      usept.y = pt.y + r2.ul.y;

      // okay, we have to fixup the fake_rects for high resolution positioning, thus the offset
      Rect userect;
      Point offset;
      offset.x = 0;
      offset.y = r.ul.y;

      // query nanites?
      RectOffsettedRect(&fake_rects[0], offset, &userect);

      if (RectTestPt(&userect, usept))
      {
         ObjID o = pShockPlayer->GetEquip(PlayerObject(), kEquipFakeNanites);
         ClearCursor();
         if (o != OBJ_NULL)
         {
            ShockSplitStack(o, TRUE);
         }
      }
   }
   return(TRUE);
}

//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockMetersCheckTransp(Point p)
{
   /*
   bool retval = FALSE;
   int pix = HandleGetPix(gHndBio,p);
   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
   */
   return(FALSE);
}
