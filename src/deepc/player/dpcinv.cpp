/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover inventory system stuff
#include <2d.h>
#include <res.h>
#include <mprintf.h>

extern "C" 
{
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

#include <dpcprop.h>
#include <dpcinv.h>
#include <dpcincst.h>
#include <dpciface.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcfsys.h>
#include <dpcplayr.h>
#include <dpcplcst.h>
#include <dpcutils.h>
#include <dpcgame.h>
#include <dpcinvpr.h>
#include <dpccurm.h>
#include <dpclooko.h>
#include <gunprop.h>
#include <dpcscapi.h>
#include <dpctrcst.h>
#include <dpcarmpr.h>
#include <dpcifstr.h>
#include <dpcmelee.h>
#include <dpcpgapi.h>

// ui library not C++ ized properly yet 
extern "C"
{
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

// Must be last header
#include <dbmem.h>

static IRes *gHndInv   = NULL;
static IRes *gBlockHnd = NULL;

static eContainType gLastLoc;


// Inventory background.
#define INV_X1      (0)
#define INV_Y1      (0)  
#define INV_X2      (INV_X1 + 188)
#define INV_Y2      (INV_Y1 + 480)

//Point player_inv_dims = {INV_COLUMNS,INV_ROWS};
Point gPlayerMaxInv = {3, 10};

// 4, 17
// Spots where icons can go.
#define INV_ICON_X1 (INV_X1 + 36)
#define INV_ICON_Y1 (INV_X1 + 37)
#define INV_ICON_X2 (INV_ICON_X1 + INV_ICON_WIDTH  * gPlayerMaxInv.x)
#define INV_ICON_Y2 (INV_ICON_Y1 + INV_ICON_HEIGHT * gPlayerMaxInv.y)

static ObjID inv_array[MAX_INV_ITEMS];
static Rect inv_icon_rect = { {INV_ICON_X1, INV_ICON_Y1}, {INV_ICON_X2, INV_ICON_Y2} };
static Rect inv_rect      = { {INV_X1, INV_Y1}, {INV_X2, INV_Y2} };

/*
static Rect close_rect = {{5,10},{5 + 20, 10 + 21}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];
*/

// ONE_HAND_ONLY - #define this if you aren't supporting a weapon in each hand.
//#define ONE_HAND_ONLY

bool InvCheckSpace(Point space, ObjID obj, int loc);
BOOL DPCInvPlayerContainsListener(eContainsEvent event, ObjID container, ObjID containee, eContainType ctype, ContainCBData);

static grs_canvas gInvCanvas;
static grs_bitmap *gInvBitmap;
static BOOL gInvChanged;

//--------------------------------------------------------------------------------------
void DPCInvClear(void)
{
   for (int i = 0; i < MAX_INV_ITEMS; ++i)
   {
      inv_array[i] = OBJ_NULL;
   }
}
//--------------------------------------------------------------------------------------
// Reassembles inv_array from containment links to the player
void DPCInvReset(void)
{
   if (PlayerObjectExists())
   {
      DPCInvComputeObjArray(PlayerObject(), inv_array);
      DPCInvRefresh();
   }
}
//--------------------------------------------------------------------------------------
void DPCInvInit(int which)
{
   gHndInv   = LoadPCX("invback");
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

   DPCInvClear();

   sScrnMode smode;
   ScrnModeGet(&smode);
   short w = RectWidth(&inv_rect);
   short h = RectHeight(&inv_rect);

   Rect use_rect;
   use_rect.ul.x = inv_rect.ul.x;
   use_rect.ul.y = inv_rect.ul.y;
   use_rect.lr.x = use_rect.ul.x + w;
   use_rect.lr.y = use_rect.ul.y + h;
   DPCOverlaySetRect(which, use_rect);

   DPCInvReset();

   //grs_bitmap *bmp = (grs_bitmap *) gHndInv->Lock();
   gInvBitmap = gr_alloc_bitmap(BMT_FLAT16, 0, w, h); // BMF_TRANS
   //gInvBitmap->align = bmp->align;
   gr_make_canvas(gInvBitmap, &gInvCanvas);

   //gHndInv->Unlock();
}
//--------------------------------------------------------------------------------------
void DPCInvTerm(void)
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
void DPCInvListenInit()
{
   AutoAppIPtr(ContainSys);
   pContainSys->Listen(gPlayerObj, DPCInvPlayerContainsListener, NULL);
}

//--------------------------------------------------------------------------------------

#define WEAPON_SLOT_WIDTH  (33)
#define WEAPON_SLOT_HEIGHT (100)
#define WEAPON_SLOT_Y1     (192)
#define WEAPON_SLOT_Y2     (WEAPON_SLOT_Y1 + WEAPON_SLOT_HEIGHT)

#define SPECIAL_Y1        (84)
#define SPECIAL_Y2        (SPECIAL_Y1 + 33)
#define SPECIAL_WIDTH     33
#define ARMOR_SLOT_WIDTH  69
#define ARMOR_SLOT_HEIGHT 67

static Rect equip_rects[kEquipMax] =
{
   {{ 24, WEAPON_SLOT_Y1},    { 24 + WEAPON_SLOT_WIDTH, WEAPON_SLOT_Y2}},           // kEquipWeapon
   {{127, WEAPON_SLOT_Y1},    {127 + WEAPON_SLOT_WIDTH, WEAPON_SLOT_Y2}},           // kEquipWeaponAlt
   {{ 55, 132},               { 55 + ARMOR_SLOT_WIDTH,  132+ARMOR_SLOT_HEIGHT}},    // kEquipArmor
   {{564, SPECIAL_Y1},        {564 + SPECIAL_WIDTH,     SPECIAL_Y2}},               // kEquipSpecial
   {{599, SPECIAL_Y1},        {599 + SPECIAL_WIDTH,     SPECIAL_Y2}},               // kEquipSpecial2
};

void DPCInvDrawCore(void)
{
   int   dx;
   int   dy;
   Point mpt;
   Point rawpt;
   Rect r2 = DPCOverlayGetRect(kOverlayInv);
   Rect r = { {INV_X1, INV_Y1}, {RectWidth(&r2), RectHeight(&r2)} };

   AutoAppIPtr(DPCPlayer);

   Point size = ContainDimsGetSize(PlayerObject());
   AssertMsg(size.x >= 0, "DPCInvDrawCore:  inv size x < 0");
   AssertMsg(size.y >= 0, "DPCInvDrawCore:  inv size y < 0");
   
   Point drawloc = {(INV_ICON_X1), (INV_ICON_Y1)};
   Point drawpt  = r.ul;

   DrawByHandle(gHndInv,drawpt);

   DPCInvDrawObjArray(size, gPlayerMaxInv, inv_array, drawloc); 

   mouse_get_xy(&rawpt.x,&rawpt.y);
   mpt.x = rawpt.x - r2.ul.x;
   mpt.y = rawpt.y - r2.ul.y;

   // Look for mouseover in-inv objects
   ObjID o;
   if (RectTestPt(&r2,rawpt))
   {
      o = DPCInvFindObject(mpt);
      if (o != OBJ_NULL)
      {
         DPCInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
   }

   // Draw equipped items
   ObjID player = PlayerObject();
   for (int i = 0; i <= kEquipSpecial2; i++)
   {
#ifdef ONE_HAND_ONLY
      // only one hand of weapons these days
      if (i == kEquipWeaponAlt)
      {
         continue;
      }
#endif // ONE_HAND_ONLY

      o = pDPCPlayer->GetEquip(player, (ePlayerEquip)i);

      if (RectTestPt(&equip_rects[i],mpt))
      {
         DPCInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
      
      if (o != OBJ_NULL)
      {
         dx = r.ul.x + equip_rects[i].ul.x + 1;
         dy = r.ul.y + equip_rects[i].ul.y + 1;
         DPCInvObjDraw(o, dx, dy);

         if (i == kEquipArmor)
         {
            // draw in some stats for effectiveness of armor
            char format[128];
            char temp[128];
            sArmor *pArmor;
            int w;
            int h;
            float val = 0.0f;
            dy = r.ul.y + equip_rects[i].ul.y + 14;
            ArmorGet(o, &pArmor);
            
            const int kNumArmorTypes = 3;
            const float* pArmorTable[kNumArmorTypes] = {&(pArmor->m_combat), &(pArmor->m_radiation), &(pArmor->m_toxic)};

            for (int j = 0; j < kNumArmorTypes; j++)
            {
               val = *pArmorTable[j];
               if (val == 0.0f)
               {
                  continue;
               }

               DPCStringFetch(format, sizeof(format), "Armor", "misc", j+1);
               sprintf(temp, format, val);
               w = gr_font_string_width(gDPCFont,temp);
               h = gr_font_string_height(gDPCFont,temp);
               dx = r.ul.x + equip_rects[i].ul.x + ((RectWidth(&equip_rects[i]) - w) / 2);
               gr_font_string(gDPCFont, temp, dx, dy);
               dy += h + 3;
            }
         }
      }
   }
}

//--------------------------------------------------------------------------------------
void DPCInvDraw(unsigned long inDeltaTicks)
{
   Rect r = DPCOverlayGetRect(kOverlayInv);
   if (gInvChanged)
   {
      gr_push_canvas(&gInvCanvas);
      DPCInvDrawCore();
      gr_pop_canvas();
      gInvChanged = FALSE;
   }
   else
   {
      // determine what is under the mouse
      Point rawpt;
      ObjID o = OBJ_NULL;
      AutoAppIPtr(DPCPlayer);

      mouse_get_xy(&rawpt.x,&rawpt.y);
      Point mpt = {(rawpt.x - r.ul.x), (rawpt.y - r.ul.y)};

      if ((DPC_cursor_mode == SCM_DRAGOBJ) && (gLastLoc != ECONTAIN_NULL))
      {
         Point dims = ContainDimsGetSize(PlayerObject());
         int slot = DPCInvFindObjSlot(dims, rawpt, drag_obj);
         if (slot != gLastLoc)
         {
            gLastLoc = ECONTAIN_NULL;
         }
      }

      // look for mouseover in-inv objects
      if (RectTestPt(&r,rawpt))
      {
         o = DPCInvFindObject(mpt);
      }

      BOOL inrect = FALSE;
      for (int i = 0; (o == OBJ_NULL) && (i <= kEquipSpecial2); i++)
      {
#ifdef ONE_HAND_ONLY
         // only one hand of weapons these days
         if (i == kEquipWeaponAlt)
         {
            continue;
         }
#endif // ONE_HAND_ONLY
         if (RectTestPt(&equip_rects[i],mpt))
         {
            inrect = TRUE;
            o = pDPCPlayer->GetEquip(PlayerObject(), (ePlayerEquip)i);
            break;
         }
      }

      if (o != OBJ_NULL)
      {
         DPCInterfaceMouseOver(o);
         g_ifaceFocusObj = o;
      }
   }
   gr_bitmap(gInvBitmap, r.ul.x, r.ul.y);
}

//--------------------------------------------------------------------------------------
void DPCInvObjDraw(ObjID o, int dx, int dy)
{
   Label *lp;
   char temp[255];

   // Get the right bitmap
   if (ObjGetObjIcon(o, &lp))
   {
      // find the bitmap
      IRes *hnd = LoadPCX(lp->text,"objicon\\");//);
      if (hnd != NULL)
      {
         // draw it!
         grs_bitmap *bmp = (grs_bitmap *) hnd->Lock();
         gr_bitmap(bmp, dx, dy);
         hnd->Unlock();

         // cleanup
         SafeFreeHnd(&hnd);
      }

      if (DPCObjGetQuantity(o, temp))
      {
         //w = gr_font_string_width(gDPCFont,temp);
         //gr_set_fcolor(gDPCTextColor);
         gr_font_string(gDPCFont, temp, dx + 3 , dy + 3);
      }
   }
}
//--------------------------------------------------------------------------------------
BOOL DPCObjGetQuantity(ObjID o, char *temp)
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

      quan = GunStateGetAmmo(o);
      drawquan = 1;
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
         {
            sprintf(temp,"%d^",quan);
            break;
         }
         default:
         {
            sprintf(temp,"%d",quan);
            break;
         }
      }
   }

   return(drawquan > 0);
}

//--------------------------------------------------------------------------------------
void DPCInvDrawObjArray(const Point& dims, const Point& maxdims, ObjID *objarr, const Point& startloc)
{
   int curObj = 0;
   int dx;
   int dy;
   
   int locX;
   int locY;
   
   int invX;
   int invY;
   int invW;
   int invH;
   ObjID o;
   bool *drawn = (bool *)Malloc(sizeof(bool) * dims.x * dims.y);
   for (int i=0; i < dims.x * dims.y; i++)
   {
      drawn[i] = FALSE;
   }

   for (invY=0; invY < maxdims.y; invY++)
   {
      for (invX=0; invX < maxdims.x; invX++)
      {
         dx = startloc.x + (INV_ICON_WIDTH * invX);
         dy = startloc.y + (INV_ICON_HEIGHT * invY);
   
         // @TODO:  Bodisafa 11/4/1999
         // This code draws the empty boxes.  We might use this again.
#if 0
            // draw in any "unavailable" blocks
            if ((invY >= dims.y) || (invX >= dims.x))
            {
               Point drawpt = {dx, dy};
               DrawByHandle(gBlockHnd, drawpt);
               continue;
            }
#endif // 0

         // okay, proceed along normally
         o = objarr[curObj];

         // nothing to draw if empty, or already drawn previously
         if ((o == OBJ_NULL) || drawn[curObj])
         {
            ++curObj;
            continue;
         }
         DPCInvObjDraw(o, dx, dy);

         // mark it and all of its slots as drawn
         invW = InvDimsGetWidth(o);
         invH = InvDimsGetHeight(o);
         for (locX = invX; locX < invX + invW; locX++)
         {
            for (locY = invY; locY < invY + invH; locY++)
            {
               int locIndex = locX + (dims.x * locY);
               if (locIndex < dims.x * dims.y)
               {
                  drawn[locIndex] = TRUE;
               }
               else
               {
                  Warning(("Trying to draw obj #%d (%d x %d) at loc (%d, %d)!\n",o, invW, invH, locX,locY));
               }
            }
         }

         // move on to next obj
         ++curObj;
      }
   }

   Free(drawn);
}
//--------------------------------------------------------------------------------------
// For the user-interface level, a request to put the object at this location in a container
bool SetInvObj(ObjID container, int x, int y, ObjID obj)
{
   Point dims = ContainDimsGetSize(container);
   int ul = (y * dims.x) + x;
   return(SetInvObj(container, ul,obj));
}
//--------------------------------------------------------------------------------------
// For the user-interface level, a request to put the object at this location in a container
bool SetInvObj(ObjID container, int ul, int obj)
{
   int i;
   ObjID prevobj = OBJ_NULL;
   int x1,y1;

   Point dims = ContainDimsGetSize(container);
   int x = ul % dims.x;
   int y = ul / dims.x;

   if (!InvCheckSpace(dims, obj, ul))
   {
      Warning(("SetInvObj: Obj %d does not fit at %d,%d (%d) !\n",obj, x, y, ul));
      return(FALSE);
   }

   int w = InvDimsGetWidth(obj);
   int h = InvDimsGetHeight(obj);
   ObjID *objarr = (ObjID *)Malloc(sizeof(ObjID) * (dims.x * dims.y));
   DPCInvComputeObjArray(container,objarr);

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
               DPCInvLoadCursor(OBJ_NULL);
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
      DPCInvLoadCursor(OBJ_NULL);
      pContainSys->Add(container, obj, ul, CTF_NONE);
   }

   if (prevobj != OBJ_NULL)
   {
      // remove it from the inventory 
      pContainSys->Remove(container,prevobj);
      DPCInvLoadCursor(prevobj);
   }

   Free(objarr);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
// do the simple geometry to find what slot is being clicked on, given objid on the cursor
int DPCInvFindObjSlot(Point dims, Point mpos, ObjID objid)
{
   if (!DPCOverlayCheck(kOverlayInv))
   {
      return(-1);
   }
   return(DPCInvFindObjSlotPos(dims,mpos,&inv_icon_rect,objid));
}

//--------------------------------------------------------------------------------------
// same as above, but more general
int DPCInvFindObjSlotPos(Point dims, Point mpos, Rect *pRect, ObjID objid)
{

   //mprintf("mpos = %d, %d\n",mpos.x,mpos.y);
   if (RectTestPt(pRect, mpos))
   {
      Point offset;
      if (objid == OBJ_NULL)
      {
         offset.x = 0;
         offset.y = 0;
      }
      else
      {
         // This is kind of ugly.  An attempt to deal with issues of mapping the hotspot
         // of the cursor to the upper left objslot of where it wants to go.
         const int w = InvDimsGetWidth(objid);
         const int h = InvDimsGetHeight(objid);
         offset.x = (((float)w / 2) - 0.5F) * INV_ICON_WIDTH;
         offset.y = (((float)h / 2) - 0.5F) * INV_ICON_HEIGHT;
      }

      const int x = (mpos.x - offset.x - pRect->ul.x) / INV_ICON_WIDTH;
      const int y = (mpos.y - offset.y - pRect->ul.y) / INV_ICON_HEIGHT;
      if ((x >= dims.x) || (y >= dims.y))
      {
         return(-1);
      }
      else
      {
         return((y * dims.x) + x);
      }
   }
   return(-1);
}
//--------------------------------------------------------------------------------------
ObjID DPCInvFindObject(int slot)
{
   return(inv_array[slot]);
}
//--------------------------------------------------------------------------------------
ObjID DPCInvFindObject(Point mpos)
{
   ObjID o = OBJ_NULL;
   Point dims = ContainDimsGetSize(PlayerObject());

   if (RectTestPt(&inv_icon_rect, mpos)) ///  || RectTestPt(&gunslot_rect, mpos))
   {
      int slot = DPCInvFindObjSlot(dims, mpos);
      if (slot != -1)
      {
         o = inv_array[slot];
      }
   }
   return(o);
}
//--------------------------------------------------------------------------------------
bool DPCInvLoadCursor(int o)
{
   // no effect
   /*
   if (!DPC_mouse)
   {
      if (o != OBJ_NULL)
      {
         Warning(("DPCInvLoadCursor: not in cursor mode!\n"));
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

   Label *lp;
   if (ObjGetObjIcon(o, &lp) && (DPC_cursor_mode == SCM_NORMAL))
   {
      IRes *hnd = LoadPCX(lp->text,"objicon\\");
      if (SetCursorByHandle(hnd))
      {
         drag_obj = o;
         DPC_cursor_mode = SCM_DRAGOBJ;
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
bool DPCInvAddObj(ObjID container, ObjID obj, int noDrop)
{
   AutoAppIPtr(ContainSys);
   int loc;

   // Don't do anything if it's already in the inventory; this can
   // cause duplicate links, which wreak merry hell on networking.
   if (pContainSys->Contains(container, obj))
   {
      return(TRUE);
   }

   Point dims = ContainDimsGetSize(container);
   ObjID *arr = (ObjID *)Malloc(sizeof(ObjID) * dims.x * dims.y);
   DPCInvComputeObjArray(container,arr);
   BOOL space =DPCInvFindSpace(arr, dims, obj, &loc);
   Free(arr);

   if (!space && noDrop) {
      // No room, and noDrop is on, so do nothing (from unequipping
      // when inventory is otherwise full
      return FALSE;
   }

   AutoAppIPtr(DPCPlayer);

   // if we are armor, and armor equip slot is free, then wear it
   if (ObjHasArmor(obj) && (pDPCPlayer->GetEquip(container,kEquipArmor) == OBJ_NULL))
   {
      const BOOL equipped = pDPCPlayer->Equip(container, kEquipArmor, obj, FALSE);
      if (equipped)
      {
         return(TRUE);
      }
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
         //DPCInvLoadCursor(OBJ_NULL);

         // play an error SFX too
         if (gThrowAudioHnd == -1)
         {
            sSchemaCallParams schParams;
            memset (&schParams, 0, sizeof (schParams));;
            schParams.flags    = SCH_SET_CALLBACK;
            schParams.sourceID = OBJ_NULL;
            schParams.callback = SchemaDoneCallback;

            gThrowAudioHnd = SchemaPlay((Label *)"bb11",&schParams);
         }
         //gThrowAudioHnd = SchemaPlay((Label *)"bb11",NULL);
         return(FALSE);
      }
   }

   HRESULT result = pContainSys->Add(container,obj,loc,CTF_COMBINE);
   if (result == S_OK)
   {
      if (drag_obj == obj)
      {
         DPCInvLoadCursor(OBJ_NULL);
         return(TRUE);
      }
   }
   return(FALSE);
}
//--------------------------------------------------------------------------------------
bool DPCInvFindSpace(ObjID *arr, Point dims, ObjID obj, int *slot)
{
   int i;
   int x;
   int y;
   int x1;
   int y1;
   int w;
   int h;

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
bool DPCInvPayNanites(int quan)
{
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(ObjectSystem);   
   ObjID arch  = pObjectSystem->GetObjectNamed("Nanites");
   HRESULT res = pContainSys->CombineAdd(PlayerObject(),arch,-quan);
   if (res == S_OK)
   {
      return(TRUE);
   }
   else
   {
      return(FALSE);
   }
}
//--------------------------------------------------------------------------------------
int DPCInvNaniteTotal(void)
{
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(ObjectSystem);   
   ObjID arch = pObjectSystem->GetObjectNamed("Nanites");
   int retval = pContainSys->CombineCount(PlayerObject(),arch);
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
   r = DPCOverlayGetRect(kOverlayInv);

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
void DPCInvStateChange(int which)
{
   gInvChanged = TRUE;
   if (DPCOverlayCheck(which))
   {
      BuildInterfaceButtons(); // just got put up
   }
   else
   {
      DestroyInterfaceButtons(); // just got taken down
   }
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool DPCInvCheckTransp(Point pt)
{
   // for now, always return false so we don't get tricked by the transparency
   // in the interlacing.
   return(FALSE);
   /*
   bool retval = FALSE;
   Point p;
   Rect r;
   r = DPCOverlayGetRect(kOverlayInv);
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
bool DPCInvHandleMouse(Point pos) 
{
   return(TRUE);
}

//--------------------------------------------------------------------------------------
bool DPCInvDoubleClick(Point pos)
{
   if (DPC_cursor_mode == SCM_NORMAL)
   {
      gInvChanged = TRUE;

      // mprintf("double click!\n");
      ObjID o = DPCInvFindObject(pos);
      if (o == OBJ_NULL)
      {
         int i;
         AutoAppIPtr(DPCPlayer);
         for (i=0; i < kEquipMax; i++)
         {
            if (RectTestPt(&equip_rects[i],pos))
            {
               ePlayerEquip slot = (ePlayerEquip)i;

               o = pDPCPlayer->GetEquip(PlayerObject(), slot);
            }
         }
      }
      if (o != OBJ_NULL)
      {
         // direct in-inv usage
         frobInvSelectObj = o;
         DPCDoFrob(TRUE);
         //mprintf("inv frobbing objid %d\n",o);
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
void DPCInvTakeMe(ObjID obj)
{
   DPCInvLoadCursor(obj);
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
void DPCSplitStack(ObjID obj, BOOL leaveEmpty)
{
   if (obj == OBJ_NULL)
   {
      return;
   }

   AutoAppIPtr(ObjectSystem);
   int maxobj = pObjectSystem->MaxObjID();
   int currobj = pObjectSystem->ActiveObjects();
   if (currobj > maxobj * 9 / 10)
   {
      char temp[255];
      DPCStringFetch(temp, sizeof(temp), "TooManyObjs", "misc");
      DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
      return;
   }

   gInvChanged = TRUE;

   int increm = 1;
   int num    = 0;
   gPropStackIncrem->Get(obj,&increm);

   // the case where we take all and leave nothing behind
   if (!gStackCountProp->Get(obj,&num) || (num <= increm))
   {
      if (leaveEmpty)
      {
         increm = num;
      }
      else
      {
         // Just take the whole stack
         DPCInvTakeMe(obj);
         return;
      }
   }

   // okay, we are actually splitting
   ObjID newobj;
   AutoAppIPtr(ContainSys);

   // this will create a new object as well
   if (leaveEmpty)
   {
      newobj = pContainSys->RemoveFromStack(obj, kStackRemoveLeaveEmpty, increm);
   }
   else
   {
      newobj = pContainSys->RemoveFromStack(obj, 0, increm);
   }
   ObjSetHasRefs(newobj,FALSE);
   PhysDeregisterModel(newobj);

   // put it on our cursor, voila!
   DPCInvLoadCursor(newobj);
}

//--------------------------------------------------------------------------------------
bool DPCInvDragDrop(Point pos, BOOL start)
{
   ObjID o, prevobj;
   ObjID player = PlayerObject();
   AutoAppIPtr(DPCPlayer);

   gInvChanged = TRUE;

   // first off, are we over an equip slot?
   for (int i=0; i < kEquipMax; i++)
   {
      if (RectTestPt(&equip_rects[i],pos))
      {
         ePlayerEquip slot = (ePlayerEquip)i;

         prevobj = pDPCPlayer->GetEquip(player, slot);
         // @TODO:  Bodisafa 10/30/1999
         // This should be a lookup table.
         switch (DPC_cursor_mode)
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
                  {
                     place_item = TRUE;
                  }
               }
               else
               {
                  // inv-inv tool usage
                  frobInvSelectObj = prevobj; 
                  DPCDoFrob(FALSE);

                  if (DPCScriptAllowSwap())
                  {
                     place_item = TRUE;
                  }
               }
               if (place_item)
               {
                  if (pDPCPlayer->Equip(player, slot,drag_obj,TRUE))
                  {
                     DPCInvLoadCursor(OBJ_NULL);
                  }
                  else
                  {
                     prevobj = OBJ_NULL;  // leave the old obj in place
                  }
               }
               else
               {
                  prevobj = OBJ_NULL;
               }
               break;
            }
            case SCM_NORMAL:
            {
               if (start)
               {
                  gLastLoc = slot;
                  if (slot != kEquipWeaponAlt)
                  {
                     if (IsMelee(prevobj)) 
                     {
                        pDPCPlayer->Equip(player, slot,OBJ_NULL,TRUE);
                     } 
                     else 
                     {
                        pDPCPlayer->SetEquip(player, slot, OBJ_NULL);
                        if (ObjHasGunState(prevobj)) {
                           AutoAppIPtr(PlayerGun);
                           pPlayerGun->Unequip(prevobj, OBJ_NULL, FALSE, FALSE);
                        }
                     }
                  }
               }
               else
               {
                  prevobj = OBJ_NULL;
               }
               break;
            }
            case SCM_LOOK:
            {
               if (!start)
               {
                  if (prevobj != OBJ_NULL)
                     DPCLookPopup(prevobj);
                  ClearCursor();
                  // leave equipped obj in place
               }
               prevobj = OBJ_NULL;
               break;
            }
         }
         
         if (prevobj != OBJ_NULL)
         {
            // handle combinables
            AutoAppIPtr(ContainSys);
            // agh, clean this up
            if (pContainSys->CombineTry(pDPCPlayer->GetEquip(player, slot), prevobj, DPCCONTAIN_PDOLLBASE + slot) == S_OK)
            {
               DPCInvLoadCursor(OBJ_NULL);
            }
            else
            {
               DPCInvLoadCursor(prevobj);
            }
         }
      }
   }
   // otherwise, do the normal thing

   o = DPCInvFindObject(pos);

   switch (DPC_cursor_mode)
   {
      case SCM_DRAGOBJ:
      {
         if (!start)
         {
            BOOL place_item = FALSE;
            Point dims = ContainDimsGetSize(PlayerObject());
            int slot = DPCInvFindObjSlot(dims,pos,drag_obj);
            if (o == OBJ_NULL)
            {
               if ((gLastLoc == ECONTAIN_NULL) || (gLastLoc != slot))
               {
                  place_item = TRUE;
               }
            }
            else
            {
               // inv-inv tool usage
               frobInvSelectObj = o; // since this is the "target" of the tool frob
               DPCDoFrob(FALSE);
               //mprintf("tool frobbing objid %d with %d\n",o,drag_obj); // DPC_cursor_useobj);

               if (DPCScriptAllowSwap())
               {
                  place_item = TRUE;
               }
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
         break;
      }
      case SCM_NORMAL:
      {
         if (o != OBJ_NULL)
         {
            if (start)
            {
               AutoAppIPtr(ContainSys);
               gLastLoc = pContainSys->IsHeld(PlayerObject(),o);
               DPCInvTakeMe(o);
            }
            break;
         }
      }
      case SCM_LOOK:
      {
         if ((o != OBJ_NULL) && !start)
         {
            DPCLookPopup(o);
            ClearCursor();
            break;
         }
      }
      case SCM_SPLIT:
      {
         if (start)
         {
            ClearCursor();
            if (o != OBJ_NULL)
            {
               DPCSplitStack(o, FALSE);
            }
            break;
         }
      }
   }
   return(TRUE);
}

//--------------------------------------------------------------------------------------
void DPCInvFillObjPos(ObjID o, ObjID *arr, int ctpos, Point dims)
{
   // assumes object will fit!

   // filter out objects that are contained by not in standard object array space
   if (ctpos >= DPCCONTAIN_PDOLLBASE)
   {
      return;
   }

   // Simple conversion, may become more complex?
   const int x1 = ctpos % dims.x;
   const int y1 = ctpos / dims.x;
   const int w = InvDimsGetWidth(o);
   const int h = InvDimsGetHeight(o);
   int y;
   for (int x = x1; x < x1 + w; x++)
   {
      for (y = y1; y < y1 + h; y++)
      {
         arr[x + (dims.x * y)] = o;
      }
   }

}
//--------------------------------------------------------------------------------------
void DPCInvComputeObjArray(ObjID o, ObjID *arr)
{
   AutoAppIPtr(ContainSys);
   if (o == OBJ_NULL)
   {
      return;
   }

   Point dims = ContainDimsGetSize(o);
   if ((dims.x == 0) || (dims.y == 0))
   {
      Warning(("DPCInvComputeObjArray:: dims is zero!\n"));
      return;
   }
   sContainIter *piter = pContainSys->IterStart(o);
   for (int i = 0; i < dims.x * dims.y; i++)
   {
      arr[i] = OBJ_NULL;
   }
   while (!piter->finished)
   {
      if (piter->type < DPCCONTAIN_PDOLLBASE)
      {
         DPCInvFillObjPos(piter->containee,arr,piter->type,dims);
      }
      pContainSys->IterNext(piter);
   }
   pContainSys->IterEnd(piter);
}
//--------------------------------------------------------------------------------------
static bool InvCheckSpace(Point space, ObjID obj, int loc)
{
   const int w = InvDimsGetWidth(obj);
   const int h = InvDimsGetHeight(obj);
   const int x = loc % space.x;
   const int y = loc / space.x;

   // okay, double-check that we fit
   if ((x + w - 1 >= space.x) || (y + h - 1 >= space.y))
   {
      return(FALSE);
   }

   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool InvCheckSpace(Point space, ObjID obj, int x, int y)
{
   const int w = InvDimsGetWidth(obj);
   const int h = InvDimsGetHeight(obj);

   // okay, double-check that we fit
   if ((x + w - 1 >= space.x) || (y + h - 1 >= space.y))
   {
      return(FALSE);
   }

   return(TRUE);
}
//--------------------------------------------------------------------------------------
// really only for use by the listener
void InvClearObj(int objid, ObjID *invarr)
{
   // finds all object slots that are equal to objid
   for (int i=0; i < MAX_INV_ITEMS; i++)
   {
      if (invarr[i] == objid)
      {
         invarr[i] = OBJ_NULL;
      }
   }
}
//--------------------------------------------------------------------------------------
BOOL DPCInvPlayerContainsListener(eContainsEvent event, ObjID container, ObjID containee, eContainType ctype, ContainCBData )
{
   if (container != PlayerObject())
   {
      // This actually gets called for any containment change, but we
      // only care about the player:
      return TRUE;
   }

   gInvChanged = TRUE;

   Point dims  = ContainDimsGetSize(container);
   BOOL retval = TRUE;
   int loc     = ctype; // simple conversion, eh?

   switch (event)
   {
      case kContainAdd:
      {

         if (loc < DPCCONTAIN_PDOLLBASE)
         {
            //mprintf("trying to place obj %d at loc %d\n",containee,loc);
            if (InvCheckSpace(dims,containee,loc))
            {
               DPCInvFillObjPos(containee, inv_array, loc, dims);
            }
            else
            {
               retval = FALSE;
            }
         }
         break;
      }

      case kContainRemove:
      {
         /*
         if (loc >= DPCCONTAIN_PDOLLBASE)
         {
            AutoAppIPtr(DPCPlayer);
            pDPCPlayer->Equip(container, (ePlayerEquip)loc,OBJ_NULL);
         }
         */
         InvClearObj(containee, inv_array);
         break;
      }
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
typedef struct
{
   ObjID obj;
   int loc;
} sResizeData;

void DPCInvResize(ObjID obj, sContainDims *olddims, sContainDims *dims)
{
   int          newloc;
   Point        oldpt;
   Point        invdims;
   AutoAppIPtr(ContainSys);

   gInvChanged = TRUE;

   // rearrange the inventory values to accomodate the new size,
   // while not appearing to move at all.

   // note that we have to gather the data in one pass, and set 
   // the data in a second pass to avoid modifying the data we
   // are iterating over.
   cSimpleDList<sResizeData> contents; 

   sContainIter *piter = pContainSys->IterStart(obj);
   while (!piter->finished)
   {
      sResizeData rsd;

      rsd.obj = piter->containee;
      rsd.loc = piter->type;
      contents.Append(rsd);

      pContainSys->IterNext(piter);
   }
   pContainSys->IterEnd(piter);

   DPCInvClear();

   // now that we have assembled a list of data, go poke
   // in the new locations for everything
   cSimpleDList<ObjID> objlist; 
   cSimpleDList<sResizeData>::cIter iter;
   for (iter = contents.Iter(); !iter.Done(); iter.Next())
   {
      sResizeData rsd = iter.Value();
      if (rsd.loc < DPCCONTAIN_PDOLLBASE)
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
            DPCInvFillObjPos(rsd.obj, inv_array, newloc, invdims);
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
      DPCInvAddObj(obj,o);
   }

   // restore the cursor if necessary
   if (save_obj != OBJ_NULL)
   {
      DPCInvLoadCursor(save_obj);
   }
}

//--------------------------------------------------------------------------------------
void DPCInvDestroyCursorObj(void)
{
   if (drag_obj == OBJ_NULL)
   {
      return;
   }

   IObjectSystem* pOS = AppGetObj(IObjectSystem);
   pOS->Destroy(drag_obj);
   SafeRelease(pOS);

   DPCInvLoadCursor(OBJ_NULL);
}
//--------------------------------------------------------------------------------------
/*
int DPCInvQuickbind(void)
{
Point rawpt, mpt;
Rect r = DPCOverlayGetRect(kOverlayInv);
eContainType retval;
ObjID o;

// look around for an object under the cursor to bind.
mouse_get_xy(&rawpt.x,&rawpt.y);
mpt.x = rawpt.x - r.ul.x;
mpt.y = rawpt.y - r.ul.y;
o = DPCInvFindObject(mpt);

// return its containtype
AutoAppIPtr(ContainSys);
retval = pContainSys->IsHeld(PlayerObject(), o);
if (retval == ECONTAIN_NULL)
  return(-1);

return(retval);
}
*/

void DPCInvRefresh(void)
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
Rect fake_rects[4] = 
{
   BUTTON_RECT(BIO_X + 183, BIO_Y + 21),
   BUTTON_RECT(BIO_X + 222, BIO_Y + 21),
   BUTTON_RECT(AMMO_X + 5,  AMMO_Y + 20),
   BUTTON_RECT(AMMO_X + 44, AMMO_Y + 20),
};
//--------------------------------------------------------------------------------------
void DPCTickerInit(int which)
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

   DPCOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
void DPCTickerTerm(void)
{
}

//--------------------------------------------------------------------------------------
void DPCTickerDraw(unsigned long inDeltaTicks)
{
   Rect r = DPCOverlayGetRect(kOverlayTicker);
   ObjID o;
   int i;
   Point mpt; // rawpt,drawpt,

   //DrawByHandle(gTickerHnd,r.ul);
   AutoAppIPtr(DPCPlayer);

   /*
   mouse_get_xy(&rawpt.x,&rawpt.y);
   mpt.x = rawpt.x - r.ul.x;
   mpt.y = rawpt.y - r.ul.y;
   */
   mouse_get_xy(&mpt.x,&mpt.y);

   for (i=kEquipFakeNanites; i <= kEquipFakeKeys; i++)
   {
      o = pDPCPlayer->GetEquip(PlayerObject(), (ePlayerEquip)i);
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
         DPCInterfaceMouseOver(o);
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
         DPCInvObjDraw(o,userect.ul.x,userect.ul.y); // drawpt.x,drawpt.y);
      }
   }
}
//--------------------------------------------------------------------------------------
EXTERN bool DPCTickerHandleMouse(Point pos)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
// puts cursor in look-at mode
void DPCSplitCursor(void)
{
   IRes *hnd;

   ClearCursor();
   DPC_cursor_mode = SCM_SPLIT;
   hnd = LoadPCX("splitcur","iface\\");
   SetCursorByHandle(hnd); 
   SafeFreeHnd(&hnd);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayInv = 
{ 
   DPCInvDraw,        // draw
   DPCInvInit,        // init
   DPCInvTerm,        // term
   DPCInvHandleMouse, // mouse
   DPCInvDoubleClick, // dclick       (really use)
   DPCInvDragDrop,    // dragdrop
   NULL,              // key
   NULL,              // bitmap
   "",                // upschema
   "",                // downschema
   DPCInvStateChange, // state
   DPCInvCheckTransp, // transparency
   0,                 // distance
   TRUE,              // needmouse
};

sOverlayFunc OverlayTicker =
{ 
   DPCTickerDraw,        // draw
   DPCTickerInit,        // init
   DPCTickerTerm,        // term
   DPCTickerHandleMouse, // mouse
   NULL,                 // dclick       (really use)
   NULL,                 // dragdrop
   NULL,                 // key
   NULL,                 // bitmap
   "",                   // upschema
   "",                   // downschema
   NULL,                 // state
   NULL,                 // transparency
   0,                    // distance
   FALSE,                // needmouse
};
