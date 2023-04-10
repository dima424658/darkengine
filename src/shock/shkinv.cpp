/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkinv.cpp,v 1.108 1999/11/19 14:57:42 adurant Exp $

#include <2d.h>
#include <res.h>
#include <mprintf.h>

extern "C" {
#include <event.h>
#include <slab.h>
#include <cursors.h>
}

#include <resapi.h>
#include <config.h>

#include <objsys.h>
#include <osysbase.h>
#include <rendprop.h>
#include <schbase.h>
#include <schema.h>
#include <scrnmode.h>
#include <scrptsrv.h>

#include <dlist.h>
#include <dlisttem.h>

#include <editor.h>
#include <guistyle.h>
#include <command.h>
#include <frobctrl.h>
#include <playrobj.h>
#include <prjctile.h>
#include <simtime.h>
#include <contain.h>
#include <combprop.h>
#include <physapi.h>

#include <keydefs.h>

#include <shkprop.h>
#include <shkinv.h>
#include <shkincst.h>
#include <shkiface.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkfsys.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkinvpr.h>
#include <shkcurm.h>
#include <shklooko.h>
#include <shkcyber.h>
//#include <shkgun.h>
#include <gunprop.h>
//#include <gunbase.h>
#include <shkscapi.h>
#include <shktrcst.h>
#include <shkarmpr.h>
#include <shkpsapi.h>
#include <shkifstr.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

// Must be last header
#include <dbmem.h>

IRes *gHndInv = NULL;
static IRes *gBlockHnd = NULL;

static eContainType gLastLoc;

#define INV_ICON_X   (4)
#define INV_ICON_Y   (17)
#define INV_X     2
//#define INV_Y     304
#define INV_Y     0 
Rect inv_icon_rect = { {INV_ICON_X, INV_ICON_Y}, {INV_ICON_X + 523,INV_ICON_Y + 98} };

/*
static Rect close_rect = {{5,10},{5 + 20, 10 + 21}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];
*/

// Shock inventory system stuff

ObjID inv_array[MAX_INV_ITEMS];
static Rect inv_rect = {{INV_X,INV_Y},{INV_X + 636, INV_Y + 121}};
//Point player_inv_dims = {INV_COLUMNS,INV_ROWS};
Point gPlayerMaxInv = {15,3};

bool InvCheckSpace(Point space, ObjID obj, int loc);
BOOL ShockInvPlayerContainsListener(eContainsEvent event, ObjID container, ObjID containee, eContainType ctype, ContainCBData);

grs_canvas gInvCanvas;
grs_bitmap *gInvBitmap;
BOOL gInvChanged;

//--------------------------------------------------------------------------------------
void ShockInvClear(void)
{
   int i;
   //if (shock_cursor_mode == SCM_NORMAL)  
   //ShockInvLoadCursor(OBJ_NULL);
   for (i=0;i < MAX_INV_ITEMS; i++)
      inv_array[i] = OBJ_NULL;
}
//--------------------------------------------------------------------------------------
// reassembles inv_array from containment links to the player
void ShockInvReset(void)
{
   if (PlayerObjectExists())
   {
      ShockInvComputeObjArray(PlayerObject(), inv_array);
      ShockInvRefresh();
   }
}
//--------------------------------------------------------------------------------------
void ShockInvInit(int which)
{

   gHndInv = LoadPCX("invback");
   gBlockHnd = LoadPCX("block");

   /*
   int i;
   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }
   */

   ShockInvClear();

   short w,h;
   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);
   w = RectWidth(&inv_rect);
   h = RectHeight(&inv_rect);
   use_rect.ul.x = (smode.w - w) / 2;
   use_rect.ul.y = inv_rect.ul.y;
   use_rect.lr.x = use_rect.ul.x + w;
   use_rect.lr.y = use_rect.ul.y + h;
   //mprintf("use_rect = %d,%d  %d,%d\n",use_rect.ul.x,use_rect.ul.y,use_rect.lr.x,use_rect.lr.y);
   ShockOverlaySetRect(which,use_rect);

   ShockInvReset();

   //grs_bitmap *bmp = (grs_bitmap *) gHndInv->Lock();
   gInvBitmap = gr_alloc_bitmap(BMT_FLAT16, 0, w, h); // BMF_TRANS
   //gInvBitmap->align = bmp->align;
   gr_make_canvas(gInvBitmap, &gInvCanvas);

   //gHndInv->Unlock();
}
//--------------------------------------------------------------------------------------
void ShockInvTerm(void)
{
   SafeFreeHnd(&gHndInv);
   SafeFreeHnd(&gBlockHnd);

   gr_free(gInvBitmap);
   /*
   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
   */
}

//--------------------------------------------------------------------------------------
void ShockInvListenInit()
{
   AutoAppIPtr(ContainSys);
   pContainSys->Listen(gPlayerObj,ShockInvPlayerContainsListener,NULL);
}

//--------------------------------------------------------------------------------------
#define ANIM_DUR  250.0F

static Rect equip_rects[kEquipMax] = {
   {{529,16},{562,116}},    // kEquipWeapon
   {{-1,-1}, {-1,-1}}, //  kEquipWeaponAlt
   {{563,16},{632,83}},    // kEquipArmor
   {{564,84},{596,117}},    // kEquipSpecial
   {{599,84},{632,117}},  // kEquipSpecial2
};

void ShockInvDrawCore(void)
{
   Point size, drawloc, drawpt;
   Point mpt,rawpt;
   Rect r2 = ShockOverlayGetRect(kOverlayInv);
   Rect r;
   tSimTime currTime = GetSimTime();
   ObjID o;
   int i,dx,dy;

   r.ul.x = 0;
   r.ul.y = 0;
   r.lr.x = RectWidth(&r2);
   r.lr.y = RectHeight(&r2);

   AutoAppIPtr(ShockPlayer);

   size = ContainDimsGetSize(PlayerObject());
   drawloc.x = r.ul.x + INV_ICON_X;
   drawloc.y = r.ul.y + INV_ICON_Y;

   drawpt = r.ul;

   DrawByHandle(gHndInv,drawpt);
   //drawloc.y = drawloc.y + (drawpt.y - inv_rect.ul.y);
   ShockInvDrawObjArray(size, gPlayerMaxInv, inv_array, drawloc); 

   //LGadDrawBox(VB(&close_button),NULL);

   mouse_get_xy(&rawpt.x,&rawpt.y);
   mpt.x = rawpt.x - r2.ul.x;
   mpt.y = rawpt.y - r2.ul.y;

   // look for mouseover in-inv objects
   if (RectTestPt(&r2,rawpt))
   {
      o = ShockInvFindObject(mpt);
      if (o != OBJ_NULL)
      {
         ShockInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
   }

   /*
   // draw the quickbind overlay
   for (i=0; i < MAX_BIND; i++)
   {
      int loc;
      char temp[128];
      int x,y;
      int dx, dy;
      loc = ShockBindVal(i);
      if ((loc != -1) && (loc < BIND_PSIBASE))
      {
         x = loc % size.x;
         y = loc / size.x;    
         sprintf(temp,"F%d",i+1);
         dx = r.ul.x + INV_ICON_X + (x * INV_ICON_WIDTH);
         dy = r.ul.y + INV_ICON_Y + (y * INV_ICON_HEIGHT);
         gr_font_string(gShockFont, temp, dx, dy);
      }
   }
   */

   // draw equipped items
   ObjID player = PlayerObject();

   for (i=0; i <= kEquipSpecial2; i++)
   {
      // only one hand of weapons these days
      if (i == kEquipWeaponAlt)
         continue;

      if (i == kEquipSpecial2)
      {
         if (!pShockPlayer->HasTrait(player,kTraitCybernetic))
         {
            drawpt = equip_rects[i].ul;
            drawpt.x += r.ul.x;
            drawpt.y += 1 + r.ul.y;
            DrawByHandle(gBlockHnd, drawpt);
            continue;
         }
      }

      o = pShockPlayer->GetEquip(player, (ePlayerEquip)i);

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

         if (i == kEquipArmor)
         {
            // draw in some stats for effectiveness of armor
            int j;
            char format[128], temp[128];
            sArmor *pArmor;
            int w,h;
            float val = 0;
            dy = r.ul.y + equip_rects[i].ul.y + 14;
            for (j=0; j < 3; j++)
            {
               ArmorGet(o,&pArmor);
               switch (j)
               {
               case 0: val = pArmor->m_combat; break;
               case 1: val = pArmor->m_radiation; break;
               case 2: val = pArmor->m_toxic; break;
               }
               if (val == 0)
                  continue;

               ShockStringFetch(format,sizeof(format),"Armor","misc",j+1);
               sprintf(temp,format,val);
               w = gr_font_string_width(gShockFont,temp);
               h = gr_font_string_height(gShockFont,temp);
               dx = r.ul.x + equip_rects[i].ul.x + ((RectWidth(&equip_rects[i]) - w) / 2);
               gr_font_string(gShockFont,temp,dx,dy);
               dy = dy + h + 3;
            }
         }
      }

   }

}
//--------------------------------------------------------------------------------------
void ShockInvDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayInv);
   if (gInvChanged)
   {
      gr_push_canvas(&gInvCanvas);
      ShockInvDrawCore();
      gr_pop_canvas();
      gInvChanged = FALSE;
   }
   else
   {
      // determine what is under the mouse
      Point rawpt, mpt;
      int i;
      ObjID o = OBJ_NULL;
      AutoAppIPtr(ShockPlayer);

      mouse_get_xy(&rawpt.x,&rawpt.y);
      mpt.x = rawpt.x - r.ul.x;
      mpt.y = rawpt.y - r.ul.y;

      if ((shock_cursor_mode == SCM_DRAGOBJ) && (gLastLoc != ECONTAIN_NULL))
      {
         int slot;
         Point dims;
         dims = ContainDimsGetSize(PlayerObject());
         slot = ShockInvFindObjSlot(dims,rawpt,drag_obj);
         if (slot != gLastLoc)
            gLastLoc = ECONTAIN_NULL;
      }

      // look for mouseover in-inv objects
      if (RectTestPt(&r,rawpt))
      {
         o = ShockInvFindObject(mpt);
      }
      
      BOOL inrect = FALSE;
      for (i=0; (o == OBJ_NULL) && (i <= kEquipSpecial2); i++)
      {
         // only one hand of weapons these days
         if (i == kEquipWeaponAlt)
            continue;

         if (RectTestPt(&equip_rects[i],mpt))
         {
            inrect = TRUE;
            o = pShockPlayer->GetEquip(PlayerObject(), (ePlayerEquip)i);
            break;
         }
      }

      if (o != OBJ_NULL)
      {
         ShockInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
      else if (inrect && (i <= kEquipSpecial2))
      {
         if ((i != kEquipSpecial2) || pShockPlayer->HasTrait(PlayerObject(),kTraitCybernetic))
            ShockStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpInv","misc",i);
      }
   }
   gr_bitmap(gInvBitmap, r.ul.x, r.ul.y);
}
//--------------------------------------------------------------------------------------
void ShockInvObjDraw(ObjID o, int dx, int dy)
{
   Label *lp;
   IRes *hnd;
   grs_bitmap *bmp;
   char temp[255];

   // get the right bitmap
   if (ObjGetObjIcon(o,&lp))
   {
      // find the bitmap
      hnd = LoadPCX(lp->text,"objicon\\");//);
      if (hnd != NULL)
      {
         // draw it!
         bmp = (grs_bitmap *) hnd->Lock();
         gr_bitmap(bmp, dx, dy);
         hnd->Unlock();

         // cleanup
         SafeFreeHnd(&hnd);
      }

      if (ShockObjGetQuantity(o,temp))
      {
         //w = gr_font_string_width(gShockFont,temp);
         //gr_set_fcolor(gShockTextColor);
         gr_font_string(gShockFont, temp, dx + 3 , dy + 3);
      }
   }
}
//--------------------------------------------------------------------------------------
BOOL ShockObjGetQuantity(ObjID o, char *temp)
{
   // stack count 
   int drawquan = 0; // from the book of draw'quan
   int quan = 0;

   if (IProperty_IsRelevant(gStackCountProp,o)) 
   {
      gStackCountProp->Get(o,&quan);
      drawquan = 1;
   }
   // ammo overlay on weapons
   else if (ObjHasGunState(o)) // IsGun(o))
   {
      int wpntype;
      g_pWeaponTypeProperty->Get(o,&wpntype);

      if (wpntype !=kWeaponPsiAmp)
      {
         quan = GunStateGetAmmo(o);
         drawquan = 1;
      }
   }
   // energy overlay
   else if (gPropEnergy->IsRelevant(o))
   {
      float energy;
      gPropEnergy->Get(o, &energy);
      quan = (int)energy;
      drawquan = 2;
   }

   if (drawquan)
   {
      switch (drawquan)
      {
      case 2:
         sprintf(temp,"%d^",quan);
         break;
      default:
         sprintf(temp,"%d",quan);
         break;
      }
   }

   return(drawquan > 0);
}
//--------------------------------------------------------------------------------------
void ShockInvDrawObjArray(Point dims, Point maxdims, ObjID *objarr, Point startloc)
{
   //Label *lp;
   int x,y,dx,dy;
   int c = 0;
   ObjID o;
   int i,j,w,h;
   bool *drawn;

   drawn = (bool *)Malloc(sizeof(bool) * dims.x * dims.y);
   for (i=0; i < dims.x * dims.y; i++)
      drawn[i] = FALSE;

   for (y=0; y < maxdims.y; y++)
   {
      for (x=0; x < maxdims.x; x++)
      {
         dx = startloc.x + (INV_ICON_WIDTH * x);
         dy = startloc.y + (INV_ICON_HEIGHT * y);

         // draw in any "unavailable" blocks
         if ((y >= dims.y) || (x >= dims.x))
         {
            Point drawpt;
            drawpt.x = dx;
            drawpt.y = dy;
            DrawByHandle(gBlockHnd, drawpt);
            continue;
         }

         // okay, proceed along normally
         o = objarr[c];

         // nothing to draw if empty, or already drawn previously
         if ((o == OBJ_NULL) || drawn[c])
         {
            c++;
            continue;
         }
         ShockInvObjDraw(o,dx,dy);

        // mark it and all of its slots as drawn
         w = InvDimsGetWidth(o);
         h = InvDimsGetHeight(o);
         for (i = x; i < x + w; i++)
         {
            for (j = y; j < y + h; j++)
            {
               int loc = i + (dims.x * j);
               if (loc < dims.x * dims.y)
                  drawn[loc] = TRUE;
               else
                  Warning(("Trying to draw obj %d (%d x %d) at %d, %d!\n",o,w,h,x,y));
            }
         }

         // move on to next obj
         c++;
      }
   }

   Free(drawn);
}
//--------------------------------------------------------------------------------------
// For the user-interface level, a request to put the object at this location in a container
bool SetInvObj(ObjID container, int x, int y, ObjID obj)
{
   Point dims;
   dims = ContainDimsGetSize(container);
   int ul = (y * dims.x) + x;
   return(SetInvObj(container, ul,obj));
}
//--------------------------------------------------------------------------------------
// For the user-interface level, a request to put the object at this location in a container
bool SetInvObj(ObjID container, int ul, int obj)
{
   int x,y;
   int w,h;
   int i;
   Point dims;
   ObjID prevobj = OBJ_NULL;
   int x1,y1;
   ObjID *objarr;

   dims = ContainDimsGetSize(container);
   x = ul % dims.x;
   y = ul / dims.x;

   if (!InvCheckSpace(dims, obj, ul))
   {
      Warning(("Obj %d does not fit at %d,%d (%d) !\n",obj,x,y,ul));
      return(FALSE);
   }

   w = InvDimsGetWidth(obj);
   h = InvDimsGetHeight(obj);
   objarr = (ObjID *)Malloc(sizeof(ObjID) * (dims.x * dims.y));
   ShockInvComputeObjArray(container,objarr);

   // find objects "underneath" this location
   AutoAppIPtr(ContainSys);
   bool keepgoing = TRUE;
   for (x1 = x; keepgoing && (x1 < x + w); x1++)
   {
      for (y1 = y; keepgoing && (y1 < y + h); y1++)
      {
         i = (y1 * dims.x) + x1;
         //mprintf("analyzing %d at loc %d\n",objarr[i],i);
         if (objarr[i] != OBJ_NULL)
         {
            bool remobj = FALSE;
            // check for combinability
            HRESULT res = pContainSys->CombineTry(objarr[i], obj, i);
            if (res == S_OK)
            {
               // okay, we've merged in our object so we're basically done
               ShockInvLoadCursor(OBJ_NULL);
               Free(objarr);
               return(TRUE);
            }
            // no combine...
            // do we already have a cursor swap candidate?  if not, make this the one
            // if so, then too bad we can't resolve this case
            if (prevobj == OBJ_NULL) 
            {
               prevobj = objarr[i];
            }
            else
            {
               if (prevobj != objarr[i])
               {
                  prevobj = OBJ_NULL;
                  keepgoing = FALSE;
               }
            }
         }
      }
   }

   if (keepgoing) // ie, we didn't abort, so put this obj in the inventory
   {
      ShockInvLoadCursor(OBJ_NULL);
      pContainSys->Add(container, obj, ul, CTF_NONE);
   }


   //mprintf("prevobj is %d\n",prevobj);
   if (prevobj != OBJ_NULL)
   {
      // remove it from the inventory 
      pContainSys->Remove(container,prevobj);
      ShockInvLoadCursor(prevobj);
   }

   Free(objarr);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
// do the simple geometry to find what slot is being clicked on, given objid on the cursor
int ShockInvFindObjSlot(Point dims, Point mpos, ObjID objid)
{
   if (!ShockOverlayCheck(kOverlayInv))
      return(-1);
   return(ShockInvFindObjSlotPos(dims,mpos,&inv_icon_rect,objid));
}

//--------------------------------------------------------------------------------------
// same as above, but more general
int ShockInvFindObjSlotPos(Point dims, Point mpos, Rect *pRect, ObjID objid)
{
   int x,y;
   Point offset;

   //mprintf("mpos = %d, %d\n",mpos.x,mpos.y);
   if (RectTestPt(pRect, mpos))
   {
      if (objid == OBJ_NULL)
      {
         offset.x = 0;
         offset.y = 0;
      }
      else
      {
         // This is kind of ugly.  An attempt to deal with issues of mapping the hotspot
         // of the cursor to the upper left objslot of where it wants to go.
         int w,h;
         w = InvDimsGetWidth(objid);
         h = InvDimsGetHeight(objid);
         offset.x = (((float)w / 2) - 0.5F) * INV_ICON_WIDTH;
         offset.y = (((float)h / 2) - 0.5F) * INV_ICON_HEIGHT;
      }

      x = (mpos.x - offset.x - pRect->ul.x) / INV_ICON_WIDTH;
      y = (mpos.y - offset.y - pRect->ul.y) / INV_ICON_HEIGHT;
      if ((x >= dims.x) || (y >= dims.y))
         return(-1);
      else
         return ((y * dims.x) + x);
   }
   return(-1);
}
//--------------------------------------------------------------------------------------
ObjID ShockInvFindObject(int slot)
{
   return (inv_array[slot]);
}
//--------------------------------------------------------------------------------------
ObjID ShockInvFindObject(Point mpos)
{
   int slot;
   ObjID o;
   Point dims;
   dims = ContainDimsGetSize(PlayerObject());

   if (RectTestPt(&inv_icon_rect, mpos)) ///  || RectTestPt(&gunslot_rect, mpos))
   {
      slot = ShockInvFindObjSlot(dims, mpos);
      if (slot == -1)
         o = OBJ_NULL;
      else
         o = inv_array[slot];
   }
   else
      o = OBJ_NULL;
   return(o);
}
//--------------------------------------------------------------------------------------
bool ShockInvLoadCursor(int o)
{
   Label *lp;
   IRes *hnd;

   // no effect
   /*
   if (!shock_mouse)
   {
      if (o != OBJ_NULL)
      {
         Warning(("ShockInvLoadCursor: not in cursor mode!\n"));
         return(FALSE); 
      }
      else
         return(TRUE);
   }
   */

   //mprintf("loading cursor with obj %d\n",o);

   if (o == OBJ_NULL)
   {
      // clear drag
      ClearCursor();
      return(TRUE);
   }

   if (ObjGetObjIcon(o, &lp) && (shock_cursor_mode == SCM_NORMAL))
   {
      hnd = LoadPCX(lp->text,"objicon\\");
      if (SetCursorByHandle(hnd))
      {
         drag_obj = o;
         shock_cursor_mode = SCM_DRAGOBJ;
         SafeFreeHnd(&hnd);
         return(TRUE);
      }
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
static int gThrowAudioHnd = -1; //  = SFX_NO_HND;
static void SchemaDoneCallback(int /*hSchema*/, ObjID /*schemaID*/, void* /*pData*/)
{
   gThrowAudioHnd = -1;
}
//--------------------------------------------------------------------------------------
// put an object in the first open slot in the inventory
bool ShockInvAddObj(ObjID container, ObjID obj)
{
   AutoAppIPtr(ContainSys);
   HRESULT result;
   Point dims;
   int loc;
   ObjID *arr;
   BOOL space;

   // Don't do anything if it's already in the inventory; this can
   // cause duplicate links, which wreak merry hell on networking.
   if (pContainSys->Contains(container, obj))
      return(TRUE);

   dims = ContainDimsGetSize(container);
   arr = (ObjID *)Malloc(sizeof(ObjID) * dims.x * dims.y);
   ShockInvComputeObjArray(container,arr);
   space =ShockInvFindSpace(arr, dims, obj, &loc);
   Free(arr);

   AutoAppIPtr(ShockPlayer);

   // if we are armor, and armor equip slot is free, then wear it
   if (ObjHasArmor(obj) && (pShockPlayer->GetEquip(container,kEquipArmor) == OBJ_NULL))
   {
      BOOL equipped;
      equipped = pShockPlayer->Equip(container,kEquipArmor,obj,FALSE);
      if (equipped)
         return(TRUE);
   }

   // okay, lets just put it the everyday joe inventory
   if (!space)
   {
      // first, check for autocombine if we are auto-adding
      if (!pContainSys->CanCombineContainer(PlayerObject(), obj, -1))
      {
         // no space, so chuck object back into world
         ThrowObj(obj, PlayerObject()); // maybe should be from container obj?
         // this line removed since ThrowObj is guaranteed to set the cursor to OBJ_NULL
         // if it is the cursor obj anyways
         //ShockInvLoadCursor(OBJ_NULL);

         // play an error SFX too
         if (gThrowAudioHnd == -1)
         {
            sSchemaCallParams schParams;
            memset (&schParams, 0, sizeof (schParams));;
            schParams.flags = SCH_SET_CALLBACK;
            schParams.sourceID = OBJ_NULL;
            schParams.callback = SchemaDoneCallback;
      
            gThrowAudioHnd = SchemaPlay((Label *)"bb11",&schParams);
         }
         //gThrowAudioHnd = SchemaPlay((Label *)"bb11",NULL);
         return(FALSE);
      }
   }

   result = pContainSys->Add(container,obj,loc,CTF_COMBINE);
   if (result == S_OK)
   {
      if (drag_obj == obj)
         ShockInvLoadCursor(OBJ_NULL);
      return(TRUE);
   }
   else
      return(FALSE);
}
//--------------------------------------------------------------------------------------
bool ShockInvFindSpace(ObjID *arr, Point dims, ObjID obj, int *slot)
{
   int i;
   int x,y,x1,y1;
   int w,h;

#ifdef PLAYTEST
   if (dims.x == 0)
   {
      Warning(("Object %d does not have a valid container dimension!\n",obj));
      return(FALSE);
   }
#endif

   for (x = 0; x < dims.x ; x++)
   {
      for (y=0; y < dims.y; y++)
      {
         i = x + (y * dims.x);
         if (arr[i] == OBJ_NULL)
         {
            // is there enough room here?
            bool space = TRUE;
            w = InvDimsGetWidth(obj);
            h = InvDimsGetHeight(obj);
            // are we too close to the edges?
            if ((x + w > dims.x) || (y + h > dims.y))
               space = FALSE;
            // now check for enough clear slots
            for (x1 = x; space && (x1 < x + w); x1++)
            {
               for (y1 = y; space && (y1 < y + h); y1++)
               {
                  if (arr[x1 + (y1 * dims.x)] != OBJ_NULL)
                     space = FALSE;
               }
            }
            if (space)
            {
               *slot = i;
               return(TRUE);
            }
         }
      }
   }
   // no matches found
   return(FALSE);
}
//--------------------------------------------------------------------------------------
bool ShockInvPayNanites(int quan)
{
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(ObjectSystem);   
   HRESULT res;

   ObjID arch = pObjectSystem->GetObjectNamed("Nanites");
   res = pContainSys->CombineAdd(PlayerObject(),arch,-quan);
   if (res == S_OK)
      return(TRUE);
   else
      return(FALSE);
}
//--------------------------------------------------------------------------------------
int ShockInvNaniteTotal(void)
{
   int retval;
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(ObjectSystem);   
   ObjID arch = pObjectSystem->GetObjectNamed("Nanites");
   retval = pContainSys->CombineCount(PlayerObject(),arch);
   return(retval);
}
//--------------------------------------------------------------------------------------
/*
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK) // || (action == BUTTONGADG_RCLICK))
   {
      uiDefer(DeferOverlayClose,(void *)kOverlayInv);
      //SchemaPlay((Label *)"subpanel_cl",NULL);
   }
   return(TRUE);
}
*/
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   /*
   Rect r;
   r = ShockOverlayGetRect(kOverlayInv);

   // set up the continue button
   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
   */
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   //LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockInvStateChange(int which)
{
   gInvChanged = TRUE;
   if (ShockOverlayCheck(which))
      BuildInterfaceButtons(); // just got put up
   else
      DestroyInterfaceButtons(); // just got taken down
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockInvCheckTransp(Point pt)
{
   // for now, always return false so we don't get tricked by the transparency
   // in the interlacing.
   return(FALSE);
   /*
   bool retval = FALSE;
   Point p;
   Rect r;
   r = ShockOverlayGetRect(kOverlayInv);
   p.x = pt.x - r.ul.x;
   p.y = pt.y - r.ul.y;
   int pix;
   pix = HandleGetPix(gHndInv,p);

   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
   */
}
//--------------------------------------------------------------------------------------
// Basic interface handling.  
//--------------------------------------------------------------------------------------
//EXTERN int hack_for_kbd_state(void);
//#define CTRL_STATES  (KBM_LCTRL|KBM_RCTRL)
bool ShockInvHandleMouse(Point pos) 
{
   /*
   ObjID o;
   //bool retval = TRUE;

   o = ShockInvFindObject(pos);
   mprintf("SIHM = %d, %d, obj = %d\n",pos.x,pos.y,o);

   switch (shock_cursor_mode)
   {
   case SCM_PSI:
      if (o != OBJ_NULL)
      {
         AutoAppIPtr(PlayerPsi);
         pPlayerPsi->PsiTarget(o);
      }
      ClearCursor();
      break;
   }
   */

   return(TRUE);
}

//--------------------------------------------------------------------------------------
bool ShockInvDoubleClick(Point pos)
{
   ObjID o;

   if (shock_cursor_mode == SCM_NORMAL)
   {
      gInvChanged = TRUE;

      // mprintf("double click!\n");
      o = ShockInvFindObject(pos);
      if (o == OBJ_NULL)
      {
         int i;
         AutoAppIPtr(ShockPlayer);
         for (i=0; i < kEquipMax; i++)
         {
            if (RectTestPt(&equip_rects[i],pos))
            {
               ePlayerEquip slot = (ePlayerEquip)i;

               o = pShockPlayer->GetEquip(PlayerObject(), slot);
            }
         }
      }
      if (o != OBJ_NULL)
      {
         // direct in-inv usage
         frobInvSelectObj = o;
         ShockDoFrob(TRUE);
         //mprintf("inv frobbing objid %d\n",o);
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
void ShockInvTakeMe(ObjID obj)
{
   ShockInvLoadCursor(obj);
   AutoAppIPtr(ContainSys);
   pContainSys->Remove(PlayerObject(),obj);
   SchemaPlay((Label *)"select_item",NULL);
}
//--------------------------------------------------------------------------------------
// user has down-alt-clicked on a stack
// if it isn't stackable, just put it on the cursor
// if it is stackable, but the increment is the whole stack, put it on the cursor
// otherwise, put on the cursor an increment's worth, and leave the rest in place.
// If leaveEmpty is TRUE, then an empty stack will be left behind if
// all are taken.
void ShockSplitStack(ObjID obj, BOOL leaveEmpty)
{
   if (obj == OBJ_NULL)
      return;

   AutoAppIPtr(ObjectSystem);
   int maxobj,currobj;
   maxobj = pObjectSystem->MaxObjID();
   currobj = pObjectSystem->ActiveObjects();
   if (currobj > maxobj * 9 / 10)
   {
      char temp[255];
      ShockStringFetch(temp,sizeof(temp),"TooManyObjs","misc");
      ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
      return;
   }

   gInvChanged = TRUE;

   int increm = 1;
   int num = 0;
   gPropStackIncrem->Get(obj,&increm);

   // the case where we take all and leave nothing behind
   if (!gStackCountProp->Get(obj,&num) || (num <= increm))
   {
      if (leaveEmpty)
      {
         increm = num;
      } else {
         // Just take the whole stack
         ShockInvTakeMe(obj);
         return;
      }
   }

   // okay, we are actually splitting
   ObjID newobj;
   AutoAppIPtr(ContainSys);

   // this will create a new object as well
   if (leaveEmpty)
      newobj = pContainSys->RemoveFromStack(obj, kStackRemoveLeaveEmpty, increm);
   else
      newobj = pContainSys->RemoveFromStack(obj, 0, increm);
   ObjSetHasRefs(newobj,FALSE);
   PhysDeregisterModel(newobj);

   // put it on our cursor, voila!
   ShockInvLoadCursor(newobj);
}
//--------------------------------------------------------------------------------------
bool ShockInvDragDrop(Point pos, BOOL start)
{
   ObjID o,prevobj;
   int i;
   ObjID player = PlayerObject();
   AutoAppIPtr(ShockPlayer);

   gInvChanged = TRUE;

   // first off, are we over an equip slot?
   for (i=0; i < kEquipMax; i++)
   {
      if (RectTestPt(&equip_rects[i],pos))
      {
         ePlayerEquip slot = (ePlayerEquip)i;

         prevobj = pShockPlayer->GetEquip(player, slot);
         switch (shock_cursor_mode)
         {
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
                  if ((gLastLoc == ECONTAIN_NULL) || (gLastLoc != slot))
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
                  if (pShockPlayer->Equip(player, slot,drag_obj,TRUE))
                     ShockInvLoadCursor(OBJ_NULL);
                  else
                     prevobj = OBJ_NULL;  // leave the old obj in place
               }
               else
                  prevobj = OBJ_NULL;
            }
            break;
         case SCM_NORMAL:
            if (start)
            {
               gLastLoc = slot;
               pShockPlayer->Equip(player, slot,OBJ_NULL,TRUE);
            }
            else
               prevobj = OBJ_NULL;
            break;
         case SCM_LOOK:
            if (!start)
            {
               if (prevobj != OBJ_NULL)
                  ShockLookPopup(prevobj);
               ClearCursor();
               // leave equipped obj in place
            }
            prevobj = OBJ_NULL;
            break;
         case SCM_PSI:
            if (!start)
            {
               if (prevobj != OBJ_NULL)
               {
                  AutoAppIPtr(PlayerPsi);
                  pPlayerPsi->PsiTarget(prevobj);
               }
               ClearCursor();
               // leave equipped obj in place
            }
            prevobj = OBJ_NULL;
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

   // otherwise, do the normal thing

   o = ShockInvFindObject(pos);

   switch (shock_cursor_mode)
   {
   case SCM_DRAGOBJ:
      {
         if (!start)
         {
            BOOL place_item = FALSE;
            int slot;
            Point dims;
            dims = ContainDimsGetSize(PlayerObject());
            slot = ShockInvFindObjSlot(dims,pos,drag_obj);
            if (o == OBJ_NULL)
            {
               if ((gLastLoc == ECONTAIN_NULL) || (gLastLoc != slot))
                  place_item = TRUE;
            }
            else
            {
               // inv-inv tool usage
               frobInvSelectObj = o; // since this is the "target" of the tool frob
               ShockDoFrob(FALSE);
               //mprintf("tool frobbing objid %d with %d\n",o,drag_obj); // shock_cursor_useobj);

               if (ShockScriptAllowSwap())
                  place_item = TRUE;
            }
            gLastLoc = ECONTAIN_NULL;
            if (place_item)
            {
               if (slot != -1)
               {
                  // make the contains system request
                  // automagically handles swapping and combining
                  SchemaPlay((Label *)"place_item", NULL);
                  SetInvObj(PlayerObject(),slot,drag_obj);
               }
            }
         }
      }
      break;
   case SCM_NORMAL:
      if (o != OBJ_NULL)
      {
         if (start)
         {
            AutoAppIPtr(ContainSys);
            gLastLoc = pContainSys->IsHeld(PlayerObject(),o);
            ShockInvTakeMe(o);
         }
      }
      break;
   case SCM_LOOK:
      if ((o != OBJ_NULL) && !start)
      {
         ShockLookPopup(o);
         ClearCursor();
      }
      break;
   case SCM_PSI:
      if ((o != OBJ_NULL) && !start)
      {
         AutoAppIPtr(PlayerPsi);
         pPlayerPsi->PsiTarget(o);
         ClearCursor();
      }
      break;
   case SCM_SPLIT:
      if (start)
      {
         ClearCursor();
         if (o != OBJ_NULL)
            ShockSplitStack(o, FALSE);
      }
      break;
   }

   return(TRUE);
}
//--------------------------------------------------------------------------------------
void ShockInvFillObjPos(ObjID o, ObjID *arr, int ctpos, Point dims)
{
   // assumes object will fit!

   // filter out objects that are contained by not in standard object array space
   if (ctpos >= SHOCKCONTAIN_PDOLLBASE)
      return;

   int x,y,w,h;
   int x1,y1;
   // simple conversion, may become more complex?
   x1 = ctpos % dims.x;
   y1 = ctpos / dims.x;
   w = InvDimsGetWidth(o);
   h = InvDimsGetHeight(o);
   for (x = x1; x < x1 + w; x++)
      for (y = y1; y < y1 + h; y++)
         arr[x + (dims.x * y)] = o;
   
}
//--------------------------------------------------------------------------------------
void ShockInvComputeObjArray(ObjID o, ObjID *arr)
{
   AutoAppIPtr(ContainSys);
   Point dims;
   sContainIter *piter;
   int i;

   if (o == OBJ_NULL)
      return;

   dims = ContainDimsGetSize(o);
   if ((dims.x == 0) || (dims.y == 0))
   {
      Warning(("ShockInvComputeObjArray:: dims is zero!\n"));
      return;
   }
   piter = pContainSys->IterStart(o);
   for (i=0; i < dims.x * dims.y; i++)
      arr[i] = OBJ_NULL;
   while (!piter->finished)
   {
      if (piter->type < SHOCKCONTAIN_PDOLLBASE)
         ShockInvFillObjPos(piter->containee,arr,piter->type,dims);
      pContainSys->IterNext(piter);
   }
   pContainSys->IterEnd(piter);
}
//--------------------------------------------------------------------------------------
static bool InvCheckSpace(Point space, ObjID obj, int loc)
{
   int x,y,w,h;
   w = InvDimsGetWidth(obj);
   h = InvDimsGetHeight(obj);
   x = loc % space.x;
   y = loc / space.x;

   // okay, double-check that we fit
   if ((x + w - 1 >= space.x) || (y + h - 1 >= space.y))
      return(FALSE);

   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool InvCheckSpace(Point space, ObjID obj, int x, int y)
{
   int w,h;
   w = InvDimsGetWidth(obj);
   h = InvDimsGetHeight(obj);

   // okay, double-check that we fit
   if ((x + w - 1 >= space.x) || (y + h - 1 >= space.y))
      return(FALSE);

   return(TRUE);
}
//--------------------------------------------------------------------------------------
// really only for use by the listener
void InvClearObj(int objid, ObjID *invarr)
{
   // finds all object slots that are equal to objid
   int i;

   for (i=0; i < MAX_INV_ITEMS; i++)
   {
      if (invarr[i] == objid)
      {
         invarr[i] = OBJ_NULL;
      }
   }
}
//--------------------------------------------------------------------------------------
BOOL ShockInvPlayerContainsListener(eContainsEvent event, ObjID container, ObjID containee, eContainType ctype, ContainCBData )
{
   if (container != PlayerObject()) {
      // This actually gets called for any containment change, but we
      // only care about the player:
      return TRUE;
   }

   gInvChanged = TRUE;

   Point dims;
   dims = ContainDimsGetSize(container);
   BOOL retval = TRUE;
   int loc;

   loc = ctype; // simple conversion, eh?

   switch (event)
   {
   case kContainAdd:
      if (loc < SHOCKCONTAIN_PDOLLBASE)
      {
         //mprintf("trying to place obj %d at loc %d\n",containee,loc);
         if (InvCheckSpace(dims,containee,loc))
            ShockInvFillObjPos(containee, inv_array, loc, dims);
         else
            retval = FALSE;
      }
      break;

   case kContainRemove:
      /*
      if (loc >= SHOCKCONTAIN_PDOLLBASE)
      {
         AutoAppIPtr(ShockPlayer);
         pShockPlayer->Equip(container, (ePlayerEquip)loc,OBJ_NULL);
      }
      */
      InvClearObj(containee, inv_array);
      break;

   }
   return(retval);
}
//--------------------------------------------------------------------------------------
typedef struct
{
   ObjID obj;
   int loc;
} sResizeData;

void ShockInvResize(ObjID obj, sContainDims *olddims, sContainDims *dims)
{
   int newloc;
   Point oldpt;
   Point invdims;
   sContainIter *piter;
   AutoAppIPtr(ContainSys);

   gInvChanged = TRUE;

   // rearrange the inventory values to accomodate the new size,
   // while not appearing to move at all.

   // note that we have to gather the data in one pass, and set 
   // the data in a second pass to avoid modifying the data we
   // are iterating over.
   cSimpleDList<sResizeData> contents; 

   piter = pContainSys->IterStart(obj);
   while (!piter->finished)
   {
      sResizeData rsd;

      rsd.obj = piter->containee;
      rsd.loc = piter->type;
      contents.Append(rsd);

      pContainSys->IterNext(piter);
   }
   pContainSys->IterEnd(piter);

   ShockInvClear();

   // now that we have assembled a list of data, go poke
   // in the new locations for everything
   cSimpleDList<ObjID> objlist; 
   cSimpleDList<sResizeData>::cIter iter;
   for (iter = contents.Iter(); !iter.Done(); iter.Next())
   {
      sResizeData rsd;
      rsd = iter.Value();
      if (rsd.loc < SHOCKCONTAIN_PDOLLBASE)
      {
         oldpt.x = rsd.loc % olddims->m_width;
         oldpt.y = rsd.loc / olddims->m_width;

         newloc = (oldpt.y * dims->m_width) + oldpt.x;

         // if the object is now in an invalid space, pitch it
         invdims.x = dims->m_width;
         invdims.y = dims->m_height;
         // compare the old location to the new dimensions
         if (InvCheckSpace(invdims,rsd.obj, oldpt.x,oldpt.y))
         {
            pContainSys->SetContainType(obj,rsd.obj, newloc);
            ShockInvFillObjPos(rsd.obj, inv_array, newloc, invdims);
         }
         else
         {
            // make a list of stuff to try and re-place in the inventory
            pContainSys->Remove(obj,rsd.obj);
            objlist.Append(rsd.obj);
         }
      }
   }

   // note if we have anything on the cursor
   ObjID save_obj = drag_obj;

   // try to replace the misfits
   cSimpleDList<ObjID>::cIter objiter;
   for (objiter = objlist.Iter(); !objiter.Done(); objiter.Next())
   {
      ObjID o;
      o = objiter.Value();
      ShockInvAddObj(obj,o);
   }

   // restore the cursor if necessary
   if (save_obj != OBJ_NULL)
   {
      ShockInvLoadCursor(save_obj);
   }
}

//--------------------------------------------------------------------------------------
void ShockInvDestroyCursorObj(void)
{
   if (drag_obj == OBJ_NULL)
      return;

   IObjectSystem* pOS = AppGetObj(IObjectSystem);
   pOS->Destroy(drag_obj);
   SafeRelease(pOS);
   
   ShockInvLoadCursor(OBJ_NULL);
}
//--------------------------------------------------------------------------------------
/*
int ShockInvQuickbind(void)
{
   Point rawpt, mpt;
   Rect r = ShockOverlayGetRect(kOverlayInv);
   eContainType retval;
   ObjID o;

   // look around for an object under the cursor to bind.
   mouse_get_xy(&rawpt.x,&rawpt.y);
   mpt.x = rawpt.x - r.ul.x;
   mpt.y = rawpt.y - r.ul.y;
   o = ShockInvFindObject(mpt);

   // return its containtype
   AutoAppIPtr(ContainSys);
   retval = pContainSys->IsHeld(PlayerObject(), o);
   if (retval == ECONTAIN_NULL)
      return(-1);

   return(retval);
}
*/

void ShockInvRefresh(void)
{
   gInvChanged = TRUE;
}

//--------------------------------------------------------------------------------------
// the area at the bottom that holds your dedicated inventory slots
//--------------------------------------------------------------------------------------
#define AMMO_X 378
#define AMMO_Y 414

#define BIO_X  2
#define BIO_Y  414

static Rect ticker_rect = {{0,0},{0,0}}; 
#define BUTTON_RECT(x,y) {{x,y},{x + INV_ICON_WIDTH, y + INV_ICON_HEIGHT}}
Rect fake_rects[4] = {
   BUTTON_RECT(BIO_X + 183, BIO_Y + 21),
   BUTTON_RECT(BIO_X + 222, BIO_Y + 21),
   BUTTON_RECT(AMMO_X + 5, AMMO_Y + 20),
   BUTTON_RECT(AMMO_X + 44, AMMO_Y + 20),
};
//--------------------------------------------------------------------------------------
void ShockTickerInit(int which)
{
   sScrnMode smode;
   Rect use_rect;

   ScrnModeGet(&smode);
   if (smode.w == 640)
   {
      use_rect.ul.x = ticker_rect.ul.x;
      use_rect.ul.y = ticker_rect.ul.y;
   }
   else
   {
      use_rect.ul.x = (smode.w - 640);
      use_rect.ul.y = (smode.h - 480);
   }

   ShockOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
void ShockTickerTerm(void)
{
}

//--------------------------------------------------------------------------------------
void ShockTickerDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayTicker);
   ObjID o;
   int i;
   Point mpt; // rawpt,drawpt,

   //DrawByHandle(gTickerHnd,r.ul);
   AutoAppIPtr(ShockPlayer);

   /*
   mouse_get_xy(&rawpt.x,&rawpt.y);
   mpt.x = rawpt.x - r.ul.x;
   mpt.y = rawpt.y - r.ul.y;
   */
   mouse_get_xy(&mpt.x,&mpt.y);

   for (i=kEquipFakeNanites; i <= kEquipFakeKeys; i++)
   {
      o = pShockPlayer->GetEquip(PlayerObject(), (ePlayerEquip)i);
      // special case for 2-handed weapons
      Rect userect;
      Point offset;
      if (i >= kEquipFakeLogs)
      {
         offset.x = r.ul.x;
         offset.y = r.ul.y;
      }
      else
      {
         offset.x = 0;
         offset.y = r.ul.y;
      }
      RectOffsettedRect(&fake_rects[i-kEquipFakeNanites], offset, &userect);

      if (RectTestPt(&userect,mpt))
      {
         ShockInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
      if (o != OBJ_NULL)
      {
         /*
         drawpt.x = fake_rects[i - kEquipFakeNanites].ul.x + 1;
         if (i >= kEquipFakeLogs)
            drawpt.x += r.ul.x;
         drawpt.y = r.ul.y + fake_rects[i - kEquipFakeNanites].ul.y + 1;
         */
         ShockInvObjDraw(o,userect.ul.x,userect.ul.y); // drawpt.x,drawpt.y);
      }
   }
}
//--------------------------------------------------------------------------------------
EXTERN bool ShockTickerHandleMouse(Point pos)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
// puts cursor in look-at mode
void ShockSplitCursor(void)
{
   IRes *hnd;

   ClearCursor();
   shock_cursor_mode = SCM_SPLIT;
   //mprintf("SLC!\n");
   hnd = LoadPCX("splitcur","iface\\");
   SetCursorByHandle(hnd); 
   SafeFreeHnd(&hnd);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayInv = { 
   ShockInvDraw, // draw
   ShockInvInit, // init
   ShockInvTerm, // term
   ShockInvHandleMouse, // mouse
   ShockInvDoubleClick, // dclick (really use)
   ShockInvDragDrop, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   ShockInvStateChange, // state
   ShockInvCheckTransp, // transparency
   0, // distance
   TRUE, // needmouse
};

sOverlayFunc OverlayTicker = { 
   ShockTickerDraw, // draw
   ShockTickerInit, // init
   ShockTickerTerm, // term
   ShockTickerHandleMouse, // mouse
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
