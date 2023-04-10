/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcont.cpp,v 1.45 2000/02/19 12:36:42 toml Exp $

// container gump
#include <2d.h>
#include <appagg.h>
#include <res.h>
#include <lgd3d.h>
#include <resapi.h>

#include <contain.h>
#include <mprintf.h>

#include <linkman.h>
#include <lnkquery.h>
#include <linkbase.h>
#include <linktype.h>
#include <relation.h>
#include <inv_rend.h>
#include <frobctrl.h>
#include <gamestr.h>
#include <dlist.h>
#include <dlisttem.h>
#include <playrobj.h>

#include <shkprop.h>
#include <shkinv.h>
#include <shkcont.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkinvpr.h>
#include <shkcurm.h>
#include <shkiface.h>
#include <shkovcst.h>
#include <shklooko.h>
#include <shkfsys.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkhplug.h>
#include <shkiftul.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

static IRes *gContainerBack;
static IRes *gBrokenHnd;
static IRes *gLockedHnd;

#define CONTDIM_NUM 16 // max size
static ObjID gContainerContents[CONTDIM_NUM];
#define CONTITEM_X   15
#define CONTITEM_Y   153
#define CONTITEM_W   INV_ICON_WIDTH
#define CONTITEM_H   INV_ICON_HEIGHT
#define CONTDESC_X   22
#define CONTDESC_Y   132

// what is the matrix?
#define MATRIX_W  142
#define MATRIX_H  138

#define CONTREND_X   17
#define CONTREND_Y   15
#define CONTREND_W   138
#define CONTREND_H   107

#define CONT_RELNAME "Contains"

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

//--------------------------------------------------------------------------------------
static invRendState *cur_inv_rend=NULL;
Rect *get_model_draw_rect(void);
void inv_show_model(void);

//--------------------------------------------------------------------------------------
void ShockContainerInit(int which)
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

   gContainerBack = LoadPCX("contain");
   gLockedHnd = LoadPCX("noget");
   gBrokenHnd = LoadPCX("bcontain");

   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockContainerTerm(void)
{
   SafeFreeHnd(&gContainerBack);
   SafeFreeHnd(&gLockedHnd);
   SafeFreeHnd(&gBrokenHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
void ShockContainerDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayContainer);
   Point dims,drawloc;
   ObjID o = ShockOverlayGetObj();
   cStr str;
   eObjState st;

   AutoAppIPtr(GameStrings);

   st = ObjGetObjState(o);

   if (st == kObjStateBroken)
   {
      DrawByHandle(gBrokenHnd,r.ul);
      LGadDrawBox(VB(&close_button),NULL);
      return;
   }

   DrawByHandle(gContainerBack,r.ul);

   inv_show_model();

   // now, look through our contents and blit
   //dims = ContainDimsGetSize(o);
   if (st == kObjStateLocked)
   {
      drawloc.x = r.ul.x + 13;
      drawloc.y = r.ul.y + 150;
      DrawByHandle(gLockedHnd, drawloc);
   }
   else
   {
      dims.x = 4; dims.y = 4; // until we have variable sized containers
      drawloc.x = r.ul.x + CONTITEM_X;
      drawloc.y = r.ul.y + CONTITEM_Y;
      ShockInvDrawObjArray(dims,dims,gContainerContents,drawloc);

      str = pGameStrings->FetchObjString(o,PROP_OBJSHORTNAME_NAME);
      char* s = (char*)(const char*)str;  // zany
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFont,s,r.ul.x + CONTDESC_X,r.ul.y + CONTDESC_Y);

      Point mpt;
      mouse_get_xy(&mpt.x,&mpt.y);
      if (RectTestPt(&r,mpt))
      {
         Point pt;
         pt.x = mpt.x - r.ul.x;
         pt.y = mpt.y - r.ul.y;
         o = ShockContainerFindObj(pt);
         ShockInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
   }

   LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
int ShockContainerFindObjSlot(Point mpos,ObjID obj)
{
   int slot;
   Point dims;

   ObjID o = ShockOverlayGetObj();
   dims = ContainDimsGetSize(o);

   Rect r;
   r.ul.x = CONTITEM_X;
   r.ul.y = CONTITEM_Y;
   r.lr.x = r.ul.x + MATRIX_W;
   r.lr.y = r.ul.y + MATRIX_H;
   slot = ShockInvFindObjSlotPos(dims,mpos,&r,obj);

   return(slot);   

   /*
   int x,y;
   x = (mpos.x - CONTITEM_X) / CONTITEM_W;
   y = (mpos.y - CONTITEM_Y) / CONTITEM_H; 
   slot = (y * dims.x) + x;
   if ((slot < 0) || (slot > CONTDIM_NUM))
      return(-1);
   else
      return (slot);
   */
}
//--------------------------------------------------------------------------------------
ObjID ShockContainerFindObj(Point mpos, int *slotp)
{
   int slot;
   ObjID o = OBJ_NULL;
   slot = ShockContainerFindObjSlot(mpos,OBJ_NULL);
   if (slot != -1)
      o = gContainerContents[slot];
   if (slotp != NULL)
      *slotp = slot;
   return(o);
}
//--------------------------------------------------------------------------------------
void ShockContainerRemove(int slot)
{
   ObjID o = ShockOverlayGetObj();
   AutoAppIPtr(ContainSys);
   if (gContainerContents[slot] != OBJ_NULL) {
      pContainSys->Remove(o, gContainerContents[slot]);
      // refresh our shadow array
      ShockInvComputeObjArray(o, gContainerContents);
   }
}
//--------------------------------------------------------------------------------------
void ShockContainerCheckRemove(ObjID o, ObjID cont)
{
   if (ShockOverlayGetObj() == cont) {
      // We're currently showing a container that something has just
      // been removed from
      Point dims = ContainDimsGetSize(cont);
      int i;
      for (i=0; i < dims.x * dims.y; i++) {
         if (gContainerContents[i] == o) {
            //mprintf("SCR-A %d\n",i);
            ShockContainerRemove(i);
         }
      }
   }
}
//--------------------------------------------------------------------------------------
bool ShockContainerHandleMouse(Point pos)
{
   ObjID o;
   int slot;

   // do nothing if object on cursor
   o = ShockContainerFindObj(pos,&slot);

   if (ObjGetObjState(ShockOverlayGetObj()) != kObjStateNormal)
      return(TRUE);

   if (shock_cursor_mode == SCM_LOOK)
   {
      if (o != OBJ_NULL)
         ShockLookPopup(o);
      ClearCursor();
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
bool ShockContainerDragDrop(Point pos, BOOL start)
{
   ObjID o;
   int slot;

   if (!start)
      return(TRUE);

   // do nothing if object on cursor
   o = ShockContainerFindObj(pos,&slot);

   if (ObjGetObjState(ShockOverlayGetObj()) != kObjStateNormal)
      return(TRUE);

   if ((shock_cursor_mode == SCM_NORMAL) && (o != OBJ_NULL))
   {
      // do a world-frob on the object
      BOOL isAutoPickup = FALSE;

      frobWorldSelectObj = o;
      ShockDoFrob(FALSE);
      gPropAutoPickup->Get(o, &isAutoPickup);
      if (isAutoPickup)
      {
         //mprintf("SCR-B %d\n",slot);
         ShockContainerRemove(slot);
      }
   }
   /*
   else if (shock_cursor_mode == SCM_DRAGOBJ)
   {
      slot = ShockContainerFindObjSlot(pos,drag_obj);
      if (slot != -1)
      {
         Point dims = {4,4};
         // put it in our container
         AutoAppIPtr(ContainSys);
         pContainSys->SetContainType(PlayerObject(),drag_obj,slot);
         ShockInvFillObjPos(drag_obj, gContainerContents, slot, dims);
         // clear the cursor
         ClearCursor();
      }
   }
   */

   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayContainer);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayContainer);

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
void ShockContainerStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();

      ObjID o;
      eObjState state;
      o = ShockOverlayGetObj();
      state = ObjGetObjState(o);
      if (state == kObjStateBroken)
      {
         ShockHRMPlugSetMode(1, o);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }
      else if (gPropHackDiff->IsRelevant(o) && (state == kObjStateLocked))
      {
         ShockHRMPlugSetMode(0, o);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
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
void ShockContainerOpen(ObjID o)
{
   sContainIter *piter;
   Point dims;
   int slot;
   int i;

   // is another container already open?  If so, close it
   if (ShockOverlayCheck(kOverlayContainer))
      ShockOverlayChange(kOverlayContainer, kOverlayModeOff);

   // open up the MFD
   ShockOverlayChangeObj(kOverlayContainer,kOverlayModeOn,o);

   // set up some data
   AutoAppIPtr(ContainSys);
   dims = ContainDimsGetSize(o);
   for (i=0; i < dims.x * dims.y; i++)
      gContainerContents[i] = OBJ_NULL;

   cSimpleDList<ObjID> contents; 
   // set the link data to arrange the objects within
   piter = pContainSys->IterStart(o);
   while (!piter->finished)
   {
      contents.Append(piter->containee);

       pContainSys->IterNext(piter);
   }
   pContainSys->IterEnd(piter);

   cSimpleDList<ObjID>::cIter iter;
   for (iter = contents.Iter(); !iter.Done(); iter.Next())
   {
      ObjID containee = iter.Value(); 
      ShockInvFindSpace(gContainerContents, dims, containee, &slot);
      pContainSys->SetContainType(o,containee,slot);
      ShockInvFillObjPos(containee, gContainerContents, slot, dims);
   }


   if (!cur_inv_rend)
   {
      ObjID o = ShockOverlayGetObj();
      cur_inv_rend=invRendBuildState(0,o,get_model_draw_rect(), NULL);
      invRendUpdateState(cur_inv_rend,INVREND_ROTATE|INVREND_HARDWARE_IMMEDIATE|INVREND_SET|INVREND_DISTANT,OBJ_NULL,NULL,NULL);
   }
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockContainerCheckTransp(Point pt)
{
   return(FALSE);
   /*
   bool retval = FALSE;
   Point p;
   Rect r;
   r = ShockOverlayGetRect(kOverlayContainer);
   p.x = pt.x - r.ul.x;
   p.y = pt.y - r.ul.y;
   int pix = HandleGetPix(gContainerBack,p);
   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
   */
}
//--------------------------------------------------------------------------------------
// and now, a bunch of stuff stolen and slightly modified from Dark's inv_hack.c
//--------------------------------------------------------------------------------------
#define OBJAREA_X 15
#define OBJAREA_Y 14
static Rect *get_model_draw_rect(void)
{
   static Rect draw_rect;

   draw_rect = ShockOverlayGetRect(kOverlayContainer);
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
      ObjID o = ShockOverlayGetObj();
      invRendUpdateState(cur_inv_rend,0,o,get_model_draw_rect(), NULL);
      invRendDrawUpdate(cur_inv_rend);
   }
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayContainer = { 
   ShockContainerDraw, // draw
   ShockContainerInit, // init
   ShockContainerTerm, // term
   ShockContainerHandleMouse, // mouse
   NULL, // dclick (really use)
   ShockContainerDragDrop, // dragdrop
   NULL, // key
   NULL, // bitmap
   "subpanel_op", // upschema
   "subpanel_cl", // downschema
   ShockContainerStateChange, // state
   ShockContainerCheckTransp, // transparency
   TRUE, // distance
   TRUE, // needmouse
};
