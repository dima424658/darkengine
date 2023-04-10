/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpdoll.cpp,v 1.27 2000/02/19 13:25:53 toml Exp $

#include <2d.h>

#include <resapi.h>
#include <mprintf.h>

#include <simtime.h>
#include <contain.h>
#include <frobctrl.h>
#include <schema.h>
#include <playrobj.h>
#include <scrnmode.h>

#include <shkanmsp.h>

#include <shkpdoll.h>
#include <shkovcst.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkinv.h>
#include <shkincst.h>
#include <shkgame.h>
#include <shkiftul.h>
#include <shkcurm.h>
#include <shkfsys.h>
#include <shkiface.h>
#include <gunprop.h>
#include <shklooko.h>
#include <shkscapi.h>

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

IRes *gPaperDollBack = NULL;
//IRes *gPaperDollStub = NULL;

static cAnimSprite *psicon;

static Rect full_rect = {{RMFD_X, RMFD_Y}, {RMFD_X + RMFD_W, RMFD_Y + RMFD_H}}; 
static Rect close_rect = {{5,8},{5 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];
static IRes *gNavDownHnd = NULL;
#define BDIM(x,y) {{x,y},{x+39,y+17}}
static Rect navdown_rect = BDIM(15,270);

static Rect equip_rects[kEquipMax] = {
   {{34,120},{100,219}},    // kEquipWeapon
   {{107,120},{174,219}},  // kEquipWeaponAlt
   {{69,48},{138,115}},    // kEquipArmor
   {{31,13},{67,48}},    // kEquipSpecial
   {{140,13},{176,48}},  // kEquipSpecial2

   {{-1,-1},{-1,-1}}, //{{86,189},{122,224}}, // kEquipPDA

   {{17,236},{49,267}}, // kEquipHack
   {{57,236},{89,267}}, // kEquipModify
   {{97,236},{129,267}}, // kEquipRepair
   {{137,236},{169,267}}, // kEquipResearch
};

//--------------------------------------------------------------------------------------
void ShockPaperDollInit(int which)
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

   gPaperDollBack = LoadPCX("equip");
   gNavDownHnd = LoadPCX("eequip1");

   sScrnMode smode;
   Rect use_rect;
   ScrnModeGet(&smode);
   use_rect.ul.x = smode.w - (640 - full_rect.ul.x);
   use_rect.ul.y = full_rect.ul.y;
   use_rect.lr.x = use_rect.ul.x + RectWidth(&full_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&full_rect);

   ShockOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
void ShockPaperDollTerm(void)
{
   SafeFreeHnd(&gPaperDollBack);
   SafeFreeHnd(&gNavDownHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
#define ANIM_DUR  250.0F
void ShockPaperDollDraw(void)
{
   Rect r;
   int i;
   ObjID o;
   tSimTime currTime = GetSimTime();
   Point drawpt;
   int dx,dy;
   AutoAppIPtr(ShockPlayer);
   ObjID player = PlayerObject();
   Point mpt;

   r = ShockOverlayGetRect(kOverlayEquip);
   drawpt = r.ul; 

   DrawByHandle(gPaperDollBack,drawpt);

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   for (i=0; i <= kEquipResearch; i++)
   {
      o = pShockPlayer->GetEquip(player, (ePlayerEquip)i);
      // special case for 2-handed weapons
      if (i == kEquipWeaponAlt)
      {
         // check if the main weapon slot is two handed
         sPlayerGunDesc *ppgd;
         ObjID mainobj;
         mainobj = pShockPlayer->GetEquip(player, kEquipWeapon);
         if (PlayerGunDescGet(mainobj, &ppgd))
         {
            if (ppgd->m_handedness == kPlayerGunTwoHanded)
            {
               o = mainobj;
            }
         }
      }
      if (RectTestPt(&equip_rects[i],mpt))
      {
         ShockInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
      if (o != OBJ_NULL)
      {
         dx = r.ul.x + equip_rects[i].ul.x + 1;
         dy = r.ul.y + equip_rects[i].ul.y + 1;
         ShockInvObjDraw(o,dx,dy);
      }
   }

  // draw in misc nav type buttons
   LGadDrawBox(VB(&close_button),NULL);
   ShockMFDNavButtonsDraw();
   // draw on top of the nav button our "down" state
   drawpt.x = navdown_rect.ul.x + r.ul.x;
   drawpt.y = navdown_rect.ul.y + r.ul.y;
   DrawByHandle(gNavDownHnd, drawpt);
}
//--------------------------------------------------------------------------------------
bool ShockPaperDollHandleMouse(Point pt)
{
   return(TRUE);
}

//--------------------------------------------------------------------------------------
bool ShockPaperDollDragDrop(Point pos, BOOL start)
{
   int i;
   ObjID prevobj;
   Rect r = ShockOverlayGetRect(kOverlayEquip);
   ObjID player = PlayerObject();
   AutoAppIPtr(ShockPlayer);

   // otherwise, handle dragging objects into slots
   // and other "real" stuff
   for (i=0; i < kEquipMax; i++)
   {
      if (RectTestPt(&equip_rects[i],pos))
      {
         ePlayerEquip slot = (ePlayerEquip)i;

         // Hmm, all this 2 handed stuff is a bit crufty... redesign at some point?
         sPlayerGunDesc *ppgd;
         if (PlayerGunDescGet(drag_obj, &ppgd))
         {
            // if the player is putting a 2 handed weapon into the alternative
            // slot, really put it in the main slot
            if ((ppgd->m_handedness == kPlayerGunTwoHanded) && (slot == kEquipWeaponAlt))
               slot = kEquipWeapon;
         }

         prevobj = pShockPlayer->GetEquip(player, slot);
         // if we are in the alt slot, and the main slot is
         // two handed, then secretly IT is the previous object
         if (slot == kEquipWeaponAlt)
         {
            ObjID mainobj = pShockPlayer->GetEquip(player, kEquipWeapon);
            sPlayerGunDesc *ppgd2;
            if (PlayerGunDescGet(mainobj, &ppgd2))
            {
               if (ppgd2->m_handedness == kPlayerGunTwoHanded)
                  prevobj = mainobj;
            }
         }
         // similarly, if we are putting a 2-hander in the main 
         // slot, then if it isn't yet specified, the prevobj comes
         // from the alternative slot
         if ((slot == kEquipWeapon) && (prevobj == OBJ_NULL))
         {
            sPlayerGunDesc *ppgd2;
            if (PlayerGunDescGet(drag_obj, &ppgd2))
            {
               if (ppgd2->m_handedness == kPlayerGunTwoHanded)
               {
                  prevobj = pShockPlayer->GetEquip(player, kEquipWeaponAlt);
               }
            }
         }
         
         switch (shock_cursor_mode)
         {
            /*
         case SCM_USEOBJ:
            if (prevobj != OBJ_NULL)
            {
               // inv-inv tool usage
               frobInvSelectObj = prevobj; 
               ShockDoFrob(FALSE);

               // but leave equipped obj in place
               prevobj = OBJ_NULL;
            }
            ClearCursor();
            break;
            */
         case SCM_DRAGOBJ:
            {
               // don't do anything on downs with something on the cursor
               if (start)
               {
                  prevobj = OBJ_NULL;
                  break;
               }

               BOOL place_item = FALSE;
               if (prevobj == OBJ_NULL)
               {
                  place_item = TRUE;
               }
               else
               {
                  // inv-inv tool usage
                  frobInvSelectObj = prevobj; 
                  ShockDoFrob(FALSE);

                  if (ShockScriptAllowSwap())
                     place_item = TRUE;
               }
               if (place_item)
               {
                  if (pShockPlayer->Equip(player, slot,drag_obj))
                  {
                     // if the previous object was two handed, and we are being put
                     // in the alt slot, clear out the main slot
                     sPlayerGunDesc *ppgd2;
                     if ((slot == kEquipWeaponAlt) && PlayerGunDescGet(prevobj, &ppgd2))
                     {
                        if (ppgd2->m_handedness == kPlayerGunTwoHanded)
                        {
                           pShockPlayer->Equip(player, kEquipWeapon, OBJ_NULL);
                        }
                     }
               
                     ShockInvLoadCursor(OBJ_NULL);
                  }
                  else
                     // leave the old obj in place
                     prevobj = OBJ_NULL;
               }
               else
                  prevobj = OBJ_NULL;
            }
            break;
         case SCM_NORMAL:
            if (start)
               pShockPlayer->Equip(player, slot,OBJ_NULL);
            break;
         case SCM_LOOK:
            if (!start)
            {
               if (prevobj != OBJ_NULL)
                  ShockLookPopup(prevobj);
               ClearCursor();
               // leave equipped obj in place
               prevobj = OBJ_NULL;
            }
            break;
         }
         if (prevobj != OBJ_NULL)
         {
            // handle combinables
            AutoAppIPtr(ContainSys);
            // agh, clean this up
            if (pContainSys->CombineTry(pShockPlayer->GetEquip(player, slot),prevobj,SHOCKCONTAIN_PDOLLBASE + slot) == S_OK)
               ShockInvLoadCursor(OBJ_NULL);
            else
               ShockInvLoadCursor(prevobj);
         }
      }
   }
   return(TRUE);
}

//--------------------------------------------------------------------------------------
bool ShockPaperDollDoubleClick(Point pos)
{
   int i;
   ObjID prevobj;
   ObjID player = PlayerObject();
   AutoAppIPtr(ShockPlayer);

   for (i=0; i < kEquipMax; i++)
   {
      if (RectTestPt(&equip_rects[i],pos))
      {
         prevobj = pShockPlayer->GetEquip(player, (ePlayerEquip)i);
         if (shock_cursor_mode == SCM_NORMAL)
         {
            if (prevobj != OBJ_NULL)
            {
               // direct in-inv usage
               frobInvSelectObj = prevobj;
               ShockDoFrob(TRUE);
            }
         }
      }
   }
   return(TRUE);
}

//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);
   uiDefer(DeferOverlayClose,(void *)kOverlayEquip);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayEquip);
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
void ShockPaperDollStateChange(int which)
{
   if (ShockOverlayCheck(which))
      BuildInterfaceButtons();
   else
      DestroyInterfaceButtons();
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockPaperDollCheckTransp(Point p)
{
   bool retval = FALSE;
   int pix;
   pix = HandleGetPix(gPaperDollBack,p);

   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
}
//--------------------------------------------------------------------------------------
