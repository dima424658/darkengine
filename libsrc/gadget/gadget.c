/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/gadget.c,v 1.56 1998/09/09 14:59:27 KEVIN Exp $

// library includes
#include <lg.h>
#ifdef _WIN32
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>
#endif

#undef BTYPE

#include <event.h>
#include <gadget.h>
#include <hotkey.h>
#include <mouse.h>
#include <string.h>
#include <lgsprntf.h>
#include <keydefs.h>
#include <math.h>
#include <limits.h>
#include <config.h>
#include <cfg.h>
#include <rect.h>
#include <cfgdbg.h>
#include <fixreal.h> // hmmm will need to deal with this eventually

// our gadget includes
#include <gcompose.h>
#include <drawelem.h>
#include <util2d.h>
#include <gadgint.h>
#include <uigadget.h>
#include <uibutton.h>

#ifndef __WATCOMC__
#define utoa ultoa
#endif

grs_bitmap ui_draw_bm;
grs_canvas ui_draw_canvas;
grs_canvas *ui_dest_canvas;
Point ui_draw_offset;
bool normal_input = TRUE;  // for shutting down input
static LGadRoot* current_root = NULL;

guiStyle* lgad_styles[MAX_PALTYPES];

short lgad_pal_blacks[MAX_PALTYPES];
short lgad_pal_text[MAX_PALTYPES];

// Are we doing 16 bit stuff?
bool lgad_use_pal16 = FALSE;
Id lgad_pal_id;

// clipboard support
// real windows support someday for this
char *(*lgad_paste_func)(void) = NULL;

// overlay support
bool (*lgad_overlay_check)(Rect *r) = NULL;
short (*lgad_overlay_add)(short x,short y, grs_bitmap *bm,OverlayStatus (*update_func)(),void *update_arg,ushort *pal) = NULL;

// recording support
void (*lgad_recorder_func)(void) = NULL;

// strings, yah.
// this defaults to RefGet
char *(*lgad_string_func)(int strref, char *buf, int bufsize) = NULL;

// memory, all alone in the lamplight
// these default to standard LG Malloc and Free
char *(*lgad_malloc)(long size) = NULL;
void (*lgad_free)(char *ptr) = NULL;



// old voyager constants
//short pal_blacks[PAL_COUNT] = { STATIC_BLACK, IFACE_BLACK, TRICORD_BLACK, 0 };
//short pal_text[PAL_COUNT] = { STATIC_TEXT, IFACE_TEXT, TRICORD_TEXT, 0 };

#define CURSOR_ALIGN 0

static ulong lgad_lookup_pal16(int paltype)
{
   ulong palid = 0;
   if (lgad_use_pal16)
   {
      palid = guiStyleGetPalette(lgad_styles[paltype]);
      if (palid == 0 && lgad_pal_id != 0)
         palid = MKREF(lgad_pal_id,paltype);
   }
   return palid;
}

static void lgad_set_pal16(int paltype, int align)
{
   if (lgad_use_pal16)
   {
      ulong palid = guiStyleGetPalette(lgad_styles[paltype]);
      if (palid == 0 && lgad_pal_id != 0)
         palid = MKREF(lgad_pal_id,paltype);

      UtilSetPalConvList(palid,align);
   }
}

static ulong get_color(LGadBox* vb, ulong stylecol, ulong override)
{
   if (override != 0)
      return override;
   return guiStyleGetColor(vb->style,stylecol);
}

static ulong get_color_default(LGadBox* vb, ulong stylecol, ulong def)
{
   ulong override = guiStyleGetColor(vb->style,stylecol);
   if (override != 0)
      return override;
   return def;
}

// Some general functions, basically exposing the UI underneath
Cursor push_cursor;
bool push_cursor_used = FALSE;

int LGadSetCursor(Ref r, int paltype, Cursor *c)
{
   Point p; // to find the hotspot
   grs_bitmap *cbm; // bitmap of art

   cbm = UtilLockBitmapRef(r);
   cbm->align = CURSOR_ALIGN;
   UtilRefAnchor(r,&p);

   lgad_set_pal16(paltype,CURSOR_ALIGN);
   uiMakeBitmapCursor(c, cbm, p);
   uiSetGlobalDefaultCursor(c);
   RefUnlock(r);

   return(0);
   
}

int LGadPushCursor(Ref r, int paltype)
{
   Point p; // to find the hotspot
   grs_bitmap *cbm; // bitmap of art

   if (push_cursor_used)
   {
      Warning(("LGadPushCursor: temp cursor already in use!\n"));
      return(-1);
   }
   cbm = UtilLockBitmapRef(r);
   cbm->align = CURSOR_ALIGN;
   UtilRefAnchor(r,&p);

   lgad_set_pal16(paltype,CURSOR_ALIGN);
   uiMakeBitmapCursor(&push_cursor, cbm, p);
   uiPushGlobalCursor(&push_cursor);
   RefUnlock(r);
   push_cursor_used = TRUE;

   return(0);
}

int LGadPopCursor()
{
   push_cursor_used = FALSE;
   // restore "normal" cursor palette
   lgad_set_pal16(VB(current_root)->paltype,CURSOR_ALIGN);

   if (uiPopGlobalCursor() == OK)
      return(0);
   else
      return(-1);
}

int LGadFlush(void)
{
   uiFlush();
   return(0);
}

/* These two functions just interpret their data as the appropriate type of Voyager input callback, then strip
our and parse the relevant data out of the mouse/key info and pass it on down.  Return value is the return
value of the inner Voyager function which is whether the input has been "claimed" by that callback and should
not be passed on to further ones. */

#pragma off(unreferenced)
bool LGadKeyHandler(uiEvent *e, Region *r, void *data)
{
   bool retval = FALSE;
   LGadBox *vb = (LGadBox *)data;
   LGadKeyCallback cb;
   uiCookedKeyEvent *cke = (uiCookedKeyEvent *)e;
   cb = vb->vkc;
   if (e->type == UI_EVENT_KBD_COOKED) 
   {
      if (cke->code & KB_FLAG_DOWN)
      {
        if (cb)
                retval = cb(cke->code,vb);
        else
                retval = FALSE;
      }
   }
   return(retval);
}

bool LGadMouseHandler(uiEvent *e, Region *r, void *data)
{
   LGadBox *vb = (LGadBox *)data;
   LGadMouseCallback cb;
   uiMouseEvent *me = (uiMouseEvent *)e;

   cb = vb->vmc;
   if ((e->type == UI_EVENT_MOUSE) && cb)
      return(cb(me->pos.x,me->pos.y,(short)me->action,me->wheel,vb));
   return(FALSE);
}

bool LGadMotionHandler(uiEvent *e, Region *r, void *data)
{
   LGadBox *vb = (LGadBox *)data;
   LGadMotionCallback cb;
   uiMouseEvent *me = (uiMouseEvent *)e;

   cb = vb->vmotc;
   if ((e->type == UI_EVENT_MOUSE_MOVE) && cb)
      return(cb(me->pos.x,me->pos.y,vb));
   return(FALSE);
}
#pragma on(unreferenced)

void GenericRegionInit(Region *reg, Rect *rct, uiSlab *slb, void *key_h, void *key_d,
   void *maus_h, void *maus_d, Cursor *c)
{
   int callid;

   if (rct==NULL)
   {
      Warning(("GenericRegInit: rct is NULL!\n"));
      return;
   }
   if (reg==NULL)
   {
      Warning(("GenericRegInit: reg is NULL!\n"));
      return;
   }

   region_create(NULL, reg, rct, 0, 0, REG_USER_CONTROLLED, NULL, NULL, NULL, NULL);
   if (key_h != NULL)
      uiInstallRegionHandler(reg,UI_EVENT_KBD_COOKED, key_h, key_d, &callid);
   if (maus_h != NULL)
      uiInstallRegionHandler(reg,UI_EVENT_MOUSE, maus_h, maus_d, &callid);
   if (slb != NULL)
   {
      uiMakeSlab(slb, reg, c);
      uiGrabSlabFocus(slb,reg, ALL_EVENTS);
   }
}

/* Builds a "root" UI area of dimension w by h, using the art of cursor c for a mouse pointer (or a color 1 dot if
c is NULL), which triggers the input callbacks if it receives input of that variety.  Returns a LGadRoot pointer
that can be used to make Voyager gadgets beneath it.  Returns NULL for failure.  If the vr argument is NULL, then
it will allocate space for a LGadRoot (although the application is subsequently responsible for deallocation).
Otherwise, the memory pointed to by vr is used.

Implementationally, under the "old" UI library this just builds a slab and an associated root region,
and since there is the tight correlation between LGadBox and Region, can return the Region and Slab pointers
as the LGadRoot pointer.  For the input handlers, we install the "generic" Voyager mouse and keyboard callbacks
that interpret their data field as the true callback to use, but can massage data appropriately before calling
the inner callback. */


#define set_region_expose(reg,exp) \
   do { \
      (reg)->status_flags |= EXPOSE_CB; \
      (reg)->expose = (exp);  \
   } while(0)


LGadRoot *LGadSetupRoot(LGadRoot *vr, short w, short h, IDataSource *pCurs, Point anchor, int paltype)
{
   grs_bitmap *c;

   if (vr == NULL)
   {
      vr = (LGadRoot *)Malloc(sizeof(LGadRoot));
      if (vr == NULL)
      {
         Warning(("LGadSetupRoot: Cannot allocate LGadRoot!\n"));
         return(NULL);
      }
      memset(vr,0,sizeof(*vr));
      vr->free_self = TRUE;
   }

   //
   // MAHK 7/19/96: Changed this so that it's possible to create a
   // cursorless root
   //

#ifdef FAIL_ON_NO_CURSOR
   if (!curs_id)
   {
      Warning(("LGadSetupRoot: cursor id %x is invalid!\n",curs_id));
      return(NULL);
   }
#endif
   vr->pCursor = pCurs;
   if (pCurs != NULL)
      c = (grs_bitmap*)IDataSource_Lock(pCurs);
   else
      c = NULL;

#ifdef FAIL_ON_NO_CURSOR
   if (c == NULL)
   {
      Warning(("LGadSetupRoot: Lock of cursor id %x yields NULL!\n",curs_id));
      return(NULL);
   }
#endif


   //   if (root_paltype != -1)
   //      Warning(("LGadSetupRoot: Root has previously been initialized!\n"));

   vr->subroot = FALSE;

   if (c != NULL)
   {
      c->align = CURSOR_ALIGN;
      lgad_set_pal16(paltype,CURSOR_ALIGN);

      vr->curs = Malloc(sizeof(Cursor));
      AssertMsg(vr->curs != NULL, "Not enough memory for cursor");
      uiMakeBitmapCursor(vr->curs, c, anchor);
   }
   else
   {
      vr->curs = NULL;
   }
   vr->root_slab = Malloc(sizeof(uiSlab));
   AssertMsg(vr->root_slab != NULL, "Not enough memory for gadget slab");

   LGadCreateBoxInternal(&vr->box,NULL, 0,0,w,h,NULL,NULL,NULL,paltype);
   vr->box.drawcall = NULL; // root_draw_func;
   //   vr->box.r.status_flags &= ~EXPOSE_CB;
   //   vr->box.r.expose = NULL;

   uiMakeSlab(vr->root_slab,&(vr->box.r),vr->curs);
   uiGrabSlabFocus(vr->root_slab,&(vr->box.r),ALL_EVENTS);
   return(vr);
}

/* Makes the given LGadRoot the new actual UI root, for input purposes.  Returns zero for success,
negatives for failure. */

int LGadSwitchRoot(LGadRoot *newroot)
{
   // MAHK 7/19/96  might as well hide the mouse and stuff
   uiHideMouse(NULL);
   uiSetCurrentSlab(newroot->root_slab);
   current_root = newroot;
   uiSetCursor();
   uiUpdateScreenSize(UI_DETECT_SCREEN_SIZE);
   uiShowMouse(NULL);
   return(0);
}

LGadRoot* LGadCurrentRoot(void)
{
   return current_root;
}

/* Will delete the LGadRoot and low-level structures.  Returns zero for success, negatives for failure. */
int LGadDestroyRoot(LGadRoot *deadroot)
{
   LGadDestroyBox(&deadroot->box, FALSE);
   // Not sure if there's a more abstract way to do this
   if (!deadroot->subroot)
   {
      uiDestroySlab(deadroot->root_slab);
      if (deadroot->pCursor != NULL)
          IDataSource_Unlock(deadroot->pCursor);
      if (deadroot->curs != NULL)
         Free(deadroot->curs);
      if (deadroot->root_slab != NULL)
         Free(deadroot->root_slab);
   }
   if (deadroot->free_self)
      Free(deadroot);
   return(0);
}

LGadRoot* LGadSetupSubRoot(LGadRoot* subroot, LGadRoot* parent, short x, short y, short w, short h)
{
   if (subroot == NULL)
   {
      subroot = Malloc(sizeof(LGadRoot));
      AssertMsg(subroot != NULL, "Could not allocate LGad subroot");
      memset(subroot,0,sizeof(*subroot));
      subroot->free_self = TRUE;
   }
   else
      subroot->free_self = FALSE;
   LGadCreateBoxInternal(&subroot->box, &parent->box.r, x,y,w,h,NULL,NULL,NULL,VB(parent)->paltype);
   subroot->box.drawcall = NULL; // root_draw_func;
   //   subroot->box.r.status_flags &= ~EXPOSE_CB;
   //   subroot->box.r.expose = NULL;
   subroot->subroot = TRUE;
   subroot->root_slab = parent->root_slab;
   subroot->curs = parent->curs;
   return subroot;
}


// *********** BOX ************

#define MAX_BOX_OVERLAYS   4
typedef struct {
   LGadBox *vb;
   grs_bitmap bm;
   char status;
   ushort id;
} OverlayInfo;
OverlayInfo box_overlays[MAX_BOX_OVERLAYS];
int num_box_overlays = 0;

#define OVERLAY_STATUS_NONE      0
#define OVERLAY_STATUS_DESTROY   1
#define OVERLAY_STATUS_FREED     2
#define OVERLAY_STATUS_REDRAW    3 // currently unimplemented

// When the renderer asks for a redraw, this is how it asks
// currently it is stupid and always sez it needs a redraw, this will eventually need
// to get changed for speed reasons
OverlayStatus LGadOverlayFunc(void *arg)
{
   OverlayInfo *oip = NULL; // what are actually drawing here?
   bool del_me = FALSE;
   int i; // iterator,duh

   // find which box_overlay corresponds to this particular thing
   for (i=0; i < num_box_overlays; i++)
      if (box_overlays[i].id == (ushort)arg)
      {
         oip = &box_overlays[i];
         break; // we've found it, no sense in continuing
      }


   if (i == num_box_overlays)
      Warning(("LGadOverlayFunc: arg %d does not map to an existing overlay!\n",(ushort)arg));

   if (oip == NULL)
   {
      Warning(("LGadOverlayFunc: oip is NULL!\n"));
      del_me = TRUE;
   }


   if (!del_me && (oip->status == OVERLAY_STATUS_DESTROY))
   {
      del_me = TRUE;
   }


   if (!del_me && (oip->vb == NULL))
   {
      Warning(("LGadOverlayFunc: oip->vb is NULL!\n"));
      del_me = TRUE;
   }
   if (!del_me && (oip->vb->drawcall == NULL))
   {
      Warning(("LGadOverlayFunc: oip->vb->drawcall is NULL!\n"));
      del_me = TRUE;
   }


   // If we are normally deleting, or detected anything bogus, remove this from the overlay system
   if (del_me)
   {
      oip->status = OVERLAY_STATUS_FREED;
      return(OverlayDelete);
   }


   gr_clear(0);
   oip->vb->drawcall(NULL,oip->vb);
//   LGadDrawBoxCanvas(oip->vb, NULL, grd_canvas, MakePoint(0,0));
   return(OverlayUpdate);
}

int LGadRemoveUnusedOverlays(void); 

// Register a LGadBox as needing to be checked for redraw in certain cases.
// I don't think this works currently for overlay areas that are not in the lower right corner
int LGadAddOverlay(LGadBox *vb)
{
   Rect nr; // "new" rectangle, in rendered area coords
   uchar *p;
   Rect r; // rect of area we care about

   // If there are no installed overlay functions, or the check_func returns FALSE, don't make an overlay
   // This is to avoid overlays when in non-render modes, etc.
   if ((lgad_overlay_check == NULL) || (lgad_overlay_check(&r) == FALSE))
      return(-1);

   // Make sure we've cleaned up the array appropriately
   LGadRemoveUnusedOverlays();

   // See if box overlays with rendered area...
   nr = *BOX_RECT(vb);
   RectMove(&nr, MakePoint(-1 * r.ul.x,-1 * r.ul.y));

   if ((nr.ul.x >= 0) && (nr.ul.y >= 0))
   {
      // We are entirely within the rendered area, so let's crank out an Overlay and set up
      // the relevant data structs
      if (num_box_overlays >= MAX_BOX_OVERLAYS)
      {
         Warning(("LGadAddOverlay: No more overlay room!\n"));
         return(-1);
      }

      // this is very stupid and needs to get fixed!!!
      p = Malloc(RectWidth(&nr) * RectHeight(&nr));

      // set up the bitmap
      // @TBD (toml 05-22-96): this bitmap should probably have a companion gr_close_bitmap(), however, since we know it's in system memory we're okay.
      gr_init_bitmap(&box_overlays[num_box_overlays].bm, p, BMT_FLAT8, BMF_TRANS, RectWidth(&nr), RectHeight(&nr));
      // set an ID so that we can find this again later
      if (num_box_overlays == 0)
         box_overlays[num_box_overlays].id = 1;
      else
         box_overlays[num_box_overlays].id = box_overlays[num_box_overlays-1].id + 1;

      if (lgad_overlay_add == NULL)
         Warning(("LGadAddOverlay: lgad_overlay_add is NULL!\n"));
      else
         lgad_overlay_add(nr.ul.x,nr.ul.y,&box_overlays[num_box_overlays].bm,LGadOverlayFunc,
            (void *)box_overlays[num_box_overlays].id,
            (lgad_use_pal16 && lgad_pal_id != 0) ? UtilGetPalConv(MKREF(lgad_pal_id,vb->paltype)) : NULL );  // pass in NULL as the 16-bit conversion if it is irrelevant

      // some bookkeeping
      box_overlays[num_box_overlays].vb = vb;
      box_overlays[num_box_overlays].status = OVERLAY_STATUS_NONE;
      num_box_overlays++;
   }
   else if ((nr.lr.x >= 0) && (nr.lr.y >= 0))
   {
      // Hmm, we partly extend into the rendered area, so let's warn about it!
      // We might be able to deal with this case fairly gracefully, but it'd be work, so
      // lets seem whether it is required first.
      Warning(("LGadAddOverlay: vb %x with dims (%d,%d),(%d,%d) partially overlays rendered area!\n",
         vb, BOX_RECT(vb)->ul.x, BOX_RECT(vb)->ul.y,BOX_RECT(vb)->lr.x,BOX_RECT(vb)->lr.y));
      return(-1);
   }

   return(0);
}

// Remove a box from the overlay list.  It is okay to call this on a LGadBox
// that you think _might_ be on the list.  Returns 0 if is successfully removed it,
// -1 if it wasn't on the list.
// Okay, in fact it doesn't actually hard core destroy the damn thing.  It just sets the status
// flag so the NEXT time we want to draw it we remove the Overlay and THEN destroy it for real.
int LGadRemoveOverlay(LGadBox *vb)
{
   int i; // iterators
   for (i=0; i < num_box_overlays; i++)
   {
      if (box_overlays[i].vb == vb)
      {
         box_overlays[i].status = OVERLAY_STATUS_DESTROY;
         return(-1);
      }
   }
   // We found no matches, but since we call this in a precautionary fashion
   // this is not worthy of warning or otherwise being annoying
   return(-1);
}

// Check for all "freed" overlays and clean up the overlay table
int LGadRemoveUnusedOverlays()
{
   int i,j; // iterators
   bool iter = TRUE;
   while (iter)
   {
      iter = FALSE;
      for (i=0; i < num_box_overlays; i++)
      {
         if (box_overlays[i].status == OVERLAY_STATUS_FREED)
         {
            if ((box_overlays[i].bm.bits)) //  && (box_overlays[i].bm.bits != overlay_bits))
               Free(box_overlays[i].bm.bits);
            if (i != num_box_overlays - 1)
               Warning(("Removing overlay from middle of array ... this has caused problems before!\n"));
            for (j=i; j < num_box_overlays-1; j++)
            {
               box_overlays[j] = box_overlays[j+1];
            }
            iter = TRUE;
            num_box_overlays--;
            break;
         }
      }
   }

   return(0);
}

void LGadInitBox(LGadBox *vb)
{
   memset(vb,0,sizeof(LGadBox));
}


bool lgadg_expose_func(Region* reg, Rect* rect)
{
   LGadBox* box = VB(reg);
   //   Rect r;
   //   RectSect(rect,reg->r,&r);
   if (box->drawcall != NULL)
      LGadDrawBoxInternal(box,NULL,rect);
   return TRUE;
}

/* This just creates a new region under the given LGadRoot, with the specified location and dimensions.
Input passthrough is much like for LGadSetupRoot.  The LGadBox passed in is totally for memory reasons, and
should point to a valid LGadBox allocation that the application is responsible for maintaining the memory of.
If vbox is NULL, then the routine will allocate the memory (although as for LGadRoot, the app is then responsible
for later freeing it).  This is the generic vanilla sub-region for input handling and building more complicated
internal interface elements.  Note that it does not specify any sort of output control flow or organization.
Returns NULL for failure, or a pointer to the LGadBox structure that is the newly created one.*/


LGadBox *LGadCreateBoxInternal(LGadBox *vbox, Region* root, short x, short y, short w, short h,
   LGadMouseCallback vmc, LGadKeyCallback vkc, DrawCallback drawcall, char paltype)
{
   Rect r; // used to hold the size of the thing
   int callid; // caller id, dummy really

   if (vbox == NULL)
   {
      vbox = (LGadBox *)Malloc(sizeof(LGadBox));
      LGadInitBox(vbox);
   }
   else
   {
      if (vbox->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateBox: vbox %x is already active!\n",vbox));
         // let's not return null.  It's way too annoying.
         //         return(NULL);
      }
   }

   if (vbox == NULL)
   {
      Warning(("LGadCreateBox: Insufficient memory for vbox malloc!\n"));
      return(NULL);
   }

   r.ul.x = x; r.ul.y = y;
   r.lr.x = x + w; r.lr.y = y + h;
   if (vbox->box_flags & BOXFLAG_ACTIVE)
      // this may or may not be a problem.  Mostly, I think it will just work.
      // But I figure having this warning here will help identify if it doesn't.
      Warning(("LGadCreateBox: box is already active!\n"));
   else
      region_create(root, &(vbox->r), &r, 0, 0, REG_USER_CONTROLLED, NULL, NULL, NULL, (void *)vbox);

   set_region_expose(&vbox->r,lgadg_expose_func);
   //   if (vkc != NULL)
   uiInstallRegionHandler(&(vbox->r),UI_EVENT_KBD_COOKED, LGadKeyHandler, (void *)vbox, &callid);
   //   if (vmc != NULL)
   uiInstallRegionHandler(&(vbox->r),UI_EVENT_MOUSE, LGadMouseHandler, (void *)vbox, &callid);
   vbox->drawcall = drawcall;
   vbox->vkc = vkc;
   vbox->vmc = vmc;
   vbox->paltype = paltype;
   vbox->box_flags |= BOXFLAG_ACTIVE;
   vbox->style = lgad_styles[paltype];

   return(vbox);
}

LGadBox *LGadCreateBox(LGadBox *vbox, LGadRoot* root, short x, short y, short w, short h,
   LGadMouseCallback vmc, LGadKeyCallback vkc, DrawCallback drawcall, char paltype)
{
   if (root == NULL)
      root = current_root;
   AssertMsg(root != NULL, "No root for box creation");
   return LGadCreateBoxInternal(vbox,&root->box.r,x,y,w,h,vmc,vkc,drawcall,paltype);
}

void LGadBoxSetUserData(LGadBox* box, void* data)
{
   box->r.user_data = data;
}

void* LGadBoxGetUserData(LGadBox* box)
{
   return box->r.user_data;
}

void LGadBoxSetMask(LGadBox* box, LGadMaskFunc func, void* data)
{
   // boxes are regions, right?
   region_set_mask(&box->r,(uiMaskFunc)func,data);
}


void LGadBoxMouseMotion(LGadBox *vbox, LGadMotionCallback vmotc)
{
   if (vbox->vmotc == NULL)
   {
      int callid;
      uiInstallRegionHandler(&(vbox->r),UI_EVENT_MOUSE_MOVE, LGadMotionHandler, (void *)vbox, &callid);
   }
   vbox->vmotc = vmotc;
}


int LGadDrawCallback(DrawCallback dcb, LGadBox *vb, void *data, Rect *r)
{
   grs_canvas *can, sub; // can contains the screen canvas to draw to, sub is the relevantly sized sub-canvas
   can = grd_canvas;
   gr_init_sub_canvas(can, &sub, r->ul.x, r->ul.y, RectWidth(r), RectHeight(r));
   gr_push_canvas(&sub);
   // deal with the sub canvas being larger than the canvas
   gr_safe_set_cliprect(max(-r->ul.x,0),max(-r->ul.y,0),
                   min(can->bm.w - r->ul.x,sub.bm.w),
                   min(can->bm.h - r->ul.y,sub.bm.h));
   dcb(data,vb);

   // draw to both video pages, if requested
   if (vb->box_flags & BOXFLAG_FLIP) {
      IDisplayDevice *dd = AppGetObj(IDisplayDevice);
      int flags;
      int count;
      // first, check if we're in page flipping mode!
      dd->lpVtbl->GetKind2(dd, NULL, &flags, NULL);
      if (flags & kDispAttemptFlippable) {
         count = dd->lpVtbl->BreakLock(dd);
         dd->lpVtbl->PageFlip(dd);
         dd->lpVtbl->RestoreLock(dd, count);
         dcb(data,vb);
      }
      SafeRelease(dd);
   }

   gr_pop_canvas();
   gr_close_sub_canvas(&sub);
   return(0);
}



// Forces the box to draw.  Pretty much just calls the draw callback.
// Returns -1 for failure.
int LGadDrawBox(LGadBox *vb, void* data)
{
   if (vb->drawcall)
   {
      LGadDrawBoxInternal(vb,data,BOX_RECT(vb));
   }
   //   else
   {
      DrawCallback dc = vb->drawcall;
      vb->drawcall = NULL;
      region_expose(&vb->r,vb->r.r);
      vb->drawcall = dc;
      return 0;
   }
}

int LGadHideBox(LGadBox* vb, bool hide)
{
   Region* reg = LGadBoxRegion(vb);
   Rect r;
   region_set_invisible(reg,hide);

   region_abs_rect(reg,reg->r,&r);
   if (hide)
      GUIErase(&r);
   region_expose(LGadBoxRegion(current_root),&r);

   return 0;
}

#define DIRECT_ALLOWED

int LGadDrawBoxInternal(LGadBox *vb, void *data,Rect* drawrect)
{
   Rect r; // rectangle to draw into
   Rect* boxr = BOX_RECT(vb);
#ifdef DIRECT_ALLOWED
   grs_canvas* save_canvas = DefaultGUIcanvas;
   GUImethods* save_methods = TheGUImethods();
#endif
   GUIcompose compose;
   int flags = ComposeFlagRead;
   bool direct =  (vb->box_flags & BOXFLAG_DIRECT) != 0;

   if (vb == NULL)
   {
      Warning(("LGadDrawBox: vb is NULL!\n"));
      return(-1);
   }

   if (vb->drawcall)
   {
      r = *boxr;
      if (drawrect != NULL)
      {
         if (!RECT_TEST_SECT(&r,drawrect))
            return 0;
         RectSect(&r,drawrect,&r);
      }

      region_abs_rect(&vb->r,&r,&r);
      RECT_MOVE(&r,ui_draw_offset);

      // setup the style for our drawelem-using friends
      ElementSetStyle(vb->style);

#ifdef DIRECT_ALLOWED
      if (direct)
      {
         SetTheGUImethods(&DefaultGUImethods);
         DefaultGUIcanvas = ui_dest_canvas;
         uiHideMouse(&r);
      }
#endif // DIRECT_ALLOWED

      {
         ulong pal = lgad_lookup_pal16(vb->paltype);

         if (pal != 0) flags |= ComposeFlagClear;

         GUIsetup(&compose,&r,flags,GUI_CANV_ANY);

         if (pal != 0)
            GUIsetpal(&compose,pal);
      }

      // build a rect as big as the box, in the same coordinates
      // as the guicompose canvas
      r.ul.x = vb->r.abs_x - r.ul.x; 
      r.ul.y = vb->r.abs_y - r.ul.y;
      r.lr.x = r.ul.x + RectWidth(boxr);
      r.lr.y = r.ul.y + RectHeight(boxr);

      LGadDrawCallback(vb->drawcall,vb,data,&r);
      GUIdone(&compose);

#ifdef DIRECT_ALLOWED
      if (direct)
      {
         r = *BOX_RECT(vb);
         uiShowMouse(&r);
         SetTheGUImethods(save_methods);
         DefaultGUIcanvas = save_canvas;
      }
#endif // DIRECT_ALLOWED

   }
   return(0);
}

// Just like LGadDrawBox, but also takes a canvas to draw into instead of just using the Screen
int LGadDrawBoxCanvas(LGadBox *vb, void *data, grs_canvas *c, Point offset)
{
   int retval;
   grs_canvas* save_canvas = DefaultGUIcanvas;
   GUImethods* save_methods = TheGUImethods();

   // setup
   DefaultGUIcanvas = c;
   SetTheGUImethods(&DefaultGUImethods);

   ui_draw_offset.x = -1 * offset.x;
   ui_draw_offset.y = -1 * offset.y;

   // go!
   retval = LGadDrawBox(vb,data);

   // cleanup
   DefaultGUIcanvas = save_canvas;
   SetTheGUImethods(save_methods);

   ui_draw_offset = MakePoint(0,0);

   return(retval);
}

/* Destroys the Box, detaching it from the UI.  If the free_self argument is TRUE, then it will
deallocate itself as well. */
// Returns -1 for failure.
int LGadDestroyBox(LGadBox *vb, bool free_self)
{

   if (vb == NULL)
   {
      Warning(("LGadDestroyBox: vb is NULL!\n"));
      return(-1);
   }
   AssertMsg(vb->box_flags & BOXFLAG_ACTIVE, "LGadDestroyBox: Cannot erase an inactive gadget!");

   LGadRemoveOverlay(vb); // safe to do even if it wasn't on the overlay list
   region_destroy(&(vb->r),FALSE);
   vb->box_flags &= ~BOXFLAG_ACTIVE;
   if (free_self)
      Free(vb);
   return(0);
}


#pragma off(unreferenced)
void LGadEraseCallback(void *data, LGadBox *vb)
{
   int color;
   if (guiStyleAvail(vb->style))
   {
      color = guiStyleGetColor(vb->style,StyleColorBlack);
   }
   else
      color = lgad_pal_blacks[vb->paltype];
   gr_clear(color);
}
#pragma on(unreferenced)

int LGadEraseBox(LGadBox *vb, bool free_self)
{
   int retval;
   AssertMsg(vb->box_flags & BOXFLAG_ACTIVE, "LGadEraseBox: Cannot erase an inactive gadget!");
   vb->drawcall = LGadEraseCallback;
   LGadDrawBox(vb,0);
   retval = LGadDestroyBox(vb,free_self);
   return(retval);
}

// *********** BUTTON ************

#ifdef OLD_LGAD_BUTTONS

#pragma off(unreferenced)
bool ButtonMouseHandler(short x, short y, short action, LGadBox *vb)
{
   bool retval=FALSE; // return value of our callback, if any
   LGadButton *vbutt = (LGadButton *)vb;  // our actual button, cast from the LGadBox param


   if (vbutt == NULL)
   {
      Warning(("ButtonMouseHandler: vbutt is NULL!\n"));
      return(FALSE);
   }

   if (action & (MOUSE_LDOWN|MOUSE_RDOWN))
   {
      vbutt->down = TRUE;
      LGadDrawBox(vb,(void *)1);
   }

   if (action & (MOUSE_LUP|MOUSE_RUP))
   {
      vbutt->down = FALSE;
      if (vbutt->buttonfunc)
         retval = vbutt->buttonfunc(action, NULL, vb);
      LGadDrawBox(vb,0);
   }
   return(retval);
}

#endif // OLD_LGAD_BUTTONS


void ButtonDraw(void *data, LGadBox *vb)
{
   LGadButton *vbutt = (LGadButton *)vb;
   DrawElemState state = (vbutt->down) ? dsDEPRESSED : dsNORMAL;
   ElementDraw(&(vbutt->gadg.draw),state,0,0,grd_canvas->bm.w,grd_canvas->bm.h);
}


static bool button_signal_handler(uiEvent* uiev, Region* reg, void* data)
{
   LGadButton* butt = (LGadButton*)reg;
   GadgEvent* ev = (GadgEvent*)uiev;

   if (ev->signaller != BUTTONGADG_SIGNALLER)
      return FALSE;

   if (butt->buttonfunc != NULL)
      butt->buttonfunc(ev->action, NULL, VB(butt));
   return FALSE; // all signal handlers return false

}

#pragma on(unreferenced)

// Deals with that pesky ability to have w or h be 0 and thus autosize.
// Note that autosizing is a bit wacky with format strings.

void LGadComputeSize(LGadButton *vbutt, short *wp, short *hp)
{
   if ((*wp <= 0) || (*hp <= 0))
   {
      short minw,minh;
      ElementSize(&BUTTON_DRAWELEM(vbutt),&minw,&minh);
      if (*wp <= 0)
         *wp = minw + abs(*wp)*2;
      if (*hp <= 0)
         *hp = minh + abs(*hp)*2;
   }
}

void LGadInitButton(LGadButton *vb)
{
   memset(vb,0,sizeof(LGadButton));
}

/* These both create a button gadget, which has some basic drawing gnosis, and can get a more focused
and easy to use mouse callback.  The button will automatically trigger drawing callbacks when pushed in
and out, and can be explicitly forced to draw with a function call.  LGadCreateButtonQuick assumes
that the vbutton pointer represents a valid app-managed LGadButton struct whose draw and buttonfunc
arguments have already been filled in.  LGadCreateButton will copy out the contents of the DrawElement
passed to it.  In either case, if vb is NULL, the function will allocate the memory for the LGadButton.
Using a 0 for w or h will cause the button to be the smallest size possible to fit the DrawElement.
*/

/* The button_flags argument can control specific drawing behavior of the button, depending on which
flags are set. */

#ifdef CREATEBUTTONNOARGS

LGadButton *LGadCreateButton(LGadButton *vb, LGadRoot *vr, short x, short y,
	short w, short h, char paltype)
{
   if (vb == NULL)
   {
      vb = (LGadButton *)Malloc(sizeof(LGadButton));
      LGadInitButton(vb);
   }
   else
   {
      if (vb->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateButton: vb %x is already active!\n",vb));
         return(NULL);
      }
   }

   if (vb == NULL)
   {
      Warning(("LGadCreateButton: vb is NULL!\n"));
      return(NULL);
   }

   LGadComputeSize(vb,&w,&h);
   LGadCreateBox((LGadBox *)vb,vr,x,y,w,h,ButtonMouseHandler,NULL,ButtonDraw,paltype);
   return(vb);
}

#endif

LGadButton *LGadCreateButtonArgs(LGadButton *vb, LGadRoot *vr, short x, short y,
     short w, short h, DrawElement *draw, LGadButtonCallback bfunc, char paltype)
{
   Rect area;
   int cookie;

   if (vb == NULL)
   {
      vb = (LGadButton *)Malloc(sizeof(LGadButton));
      LGadInitButton(vb);
   }
   if (vr == NULL)
   {
      vr = LGadCurrentRoot();
   }
   if (vr == NULL)
   {
      Warning(("No root for LGadCreateButtonArgs\n"));
      return NULL;
   }

   // autosize
   if (h <= 0 || w <= 0)
   {
      short aw,ah;
      ElementSize(draw,&aw,&ah);
      if (h <= 0) h = ah;
      if (w <= 0) w = aw;
   }

   area.ul = MakePoint(x,y);
   area.lr = MakePoint(x+w,y+h);

   ButtonGadgInit(&VB(vr)->r,&vb->gadg, &area, 0, draw);
   uiInstallRegionHandler(&VB(vb)->r,UI_EVENT_USER_DEFINED,button_signal_handler,NULL,&cookie);

   vb->down = FALSE;
   vb->back_color = 0;
   vb->buttonfunc = bfunc;
   VB(vb)->paltype = paltype;
   return vb;
}

// ********** Toggle **********

void LGadComputeVarSize(LGadToggle *vt, short *wp, short *hp)
{
   // To compute the right size, we go through all the possible values and
   // query for sizes on them, then take the largest extent.
   if ((*wp <= 0) || (*hp <= 0))
   {
      int orig_val = *vt->val_ptr; // original value, for restoring later
      int i; // iterator?  But I just met her!
      short maxw=0,maxh=0; // keep track of largest extent
      short tempw,temph; // for getting info out of LGadComputeSize
      for (i=0; i < vt->max_val; i++)
      {
         *vt->val_ptr = i;
         tempw = temph = 0;
         LGadComputeSize((LGadButton *)vt,&tempw,&temph);
         if (tempw > maxw)
            maxw = tempw;
         if (temph > maxh)
            maxh = temph;
      }
      if (*wp <= 0)
         *wp = maxw + abs(*wp)*2;
      if (*hp <= 0)
         *hp = maxh + abs(*hp)*2;
      *vt->val_ptr = orig_val;
   }
}

 /* A LGadToggle is pretty much just like a LGadButton but it can also track a variable in addition to vectoring
off to a mouse callback.  Much like for LGadRoot and LGadBox if the LGadToggle pointer handed to it is NULL, it
will allocate the memory and return a pointer to the new one.  Otherwise it will assume that the pointer handed
it is a valid LGadToggle to fill with info and use.  Note that the DRAWTYPE_VARSTRING and DRAWTYPE_VARRES are
useful to use with LGadToggle, as is DRAWTYPE_VAR.  Using a 0 for w or h will cause it to auto-size in that
dimension to the smallest size to fit the DrawElement. */
/* Negative values for w or h will autosize with a padding of the size of the absolute value passed in.
Thus using -3 for w will cause it to autosize with a margin of 3 in width. */

/* As the LGadToggle recieves up events, it will add (or subtract, on right clicks) the value of increm to
the pointer, and keep the value in the range of 0 to max_val-1.  As the button callback func is triggered for
each click, it passes in the contents of the variable as the data parameter. */

#pragma off(unreferenced)
bool ToggleMouseHandler(short x, short y, short action, LGadBox *vb)
{
   bool retval=FALSE; // return value of our callback, if any
   LGadToggle *vt = (LGadToggle *)vb;  // our actual toggle, cast from the LGadBox param

   if (vt == NULL)
   {
      Warning(("ToggleMouseHandler: vt is NULL!\n"));
      return(FALSE);
   }

   if (action & (MOUSE_LDOWN|MOUSE_RDOWN))
   {
      vt->down = TRUE;
      LGadDrawBox(vb,NULL);
   }

   if (action & (MOUSE_LUP|MOUSE_RUP))
   {
      int mul = 1;
      int *vp = vt->val_ptr;
      vt->down = FALSE;
      if (action & MOUSE_RUP)
         mul = -1;
      // Update the value in val_ptr, by incrementing (or decrementing) as appropriate and taking it
      // modulo the maximum value plus 1 (since max_val is a valid value).
      *vp = (*vp + (mul * vt->increm));
      while (*vp >= vt->max_val)
         *vp -= (vt->max_val);
      while (*vp < 0)
         *vp += (vt->max_val);
      if (vt->buttonfunc)
         retval = vt->buttonfunc(action, NULL, vb);
      LGadDrawBox(vb,NULL);
   }
   return(retval);
}

void LGadInitToggle(LGadToggle *vt)
{
   memset(vt,0,sizeof(LGadToggle));
   LGadInitButton((LGadButton *)vt);
   ElementClear(&BUTTON_DRAWELEM(vt));
}


LGadToggle *LGadCreateToggle(LGadToggle *vt, LGadRoot *vr, short x, short y, short w, short h, char paltype)
{
   if (vt == NULL)
   {
      vt = (LGadToggle *)Malloc(sizeof(LGadToggle));
      LGadInitToggle(vt);
   }
   else
   {
      if (VB(vt)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateToggle: vt %x is already active!\n",vt));
         return(NULL);
      }
   }

   if (vt == NULL)
   {
      Warning(("LGadCreateToggle: vt is NULL!\n"));
      return(NULL);
   }

   switch(BUTTON_DRAWELEM(vt).draw_type)
   {
      case DRAWTYPE_VAR:
      case DRAWTYPE_VARSTRING:
      case DRAWTYPE_VARTEXTREF:
      case DRAWTYPE_VARRES:
      case DRAWTYPE_FORMAT:
      case DRAWTYPE_FORMATREF:
         BUTTON_DRAWELEM(vt).draw_data2 = vt->val_ptr;
         break;
   }

   LGadComputeVarSize(vt,&w,&h);

   // Note that we can use ButtonDraw since once we've set the draw_data2 pointer we
   // are in all visual ways a button.
   LGadCreateBox((LGadBox *)vt,vr,x,y,w,h,ToggleMouseHandler,NULL,ButtonDraw, paltype);
   return(vt);
}

LGadToggle *LGadCreateToggleArgs(LGadToggle *vt, LGadRoot *vr, short x, short y, short w, short h,
   DrawElement *draw, LGadButtonCallback bfunc, int *val_ptr, short max_val, short increm, char paltype)
{
   if (vt == NULL)
   {
      vt = (LGadToggle *)Malloc(sizeof(LGadToggle));
      LGadInitToggle(vt);
   }
   else
   {
      if (VB(vt)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateToggleArgs: vt %x is already active!\n",vt));
         return(NULL);
      }
   }

   if (vt == NULL)
   {
      Warning(("LGadCreateToggleArgs: vt is NULL, after Malloc!\n"));
      return(NULL);
   }

   vt->increm = increm;
   vt->max_val = max_val;
   vt->val_ptr = val_ptr;
   vt->buttonfunc = bfunc;
   BUTTON_DRAWELEM(vt) = *draw;

   return(LGadCreateToggle(vt,vr,x,y,w,h,paltype));
}
#pragma on(unreferenced)

// ********** Scale **********

/* As you might expect in the progression of gadgetry, a LGadScale is much like a LGadToggle, but the value
of the variable is displayed graphically, and the user can use the mouse to cause quick changes in the
contents of that value.  Every increm value is a "pip" on the scale, so there are max_val/increm pips
going from left to right.  The DrawElement is presented above the visual scale.  As you might expect,
non-positive values for w or for h will cause it to autosize similarly as for LGadToggles. */

bool ScaleMotionHandler(short x, short y, LGadBox *vb)
{
   LGadScale *vs;
   Rect *r;
   bool retval = FALSE;
   short tw,th; // temp width and height

   vs = (LGadScale *)vb;
   if (vs->down)
   {
      r = (BOX_RECT(vb));
      // compensate for internal drawelement stuff
      ElementOffsetAlternate(&BUTTON_DRAWELEM(vs),&tw,&th);

      // If in the "scale" area, allow directly clicking on a pip
      ConfigSpew("scalemove",("%d vs %d - %d - %d = %d\n",y,r->lr.y,vs->dot_h + vs->dot_margin,th,
         r->lr.y - (vs->dot_h + vs->dot_margin) - th));

      if ((y > r->lr.y - (vs->dot_h + vs->dot_margin) - th) || (vs->scale_flags & SCALE_TITLELEFT))
      {
         int oldval,newval; // original value and new value
         oldval = *vs->val_ptr;

         // Compute x relative to the interesting part of the scale
         x = x - r->ul.x;  // to compensate for position of gadget
         if (vs->scale_flags & SCALE_TITLELEFT)
         {
            short ew,eh;
            if (vs->internal_w == 0)
               ElementSize(&BUTTON_DRAWELEM(vs),&ew,&eh);
            else
               ew = vs->internal_w;
            x = x - ew;
         }
         else
         {
            x = x - tw;
         }
         newval = (x / (vs->dot_w + vs->dot_margin)) * vs->increm;
         if (newval < 0)
            newval = 0;
         if (newval >= vs->max_val)
            newval = vs->max_val - 1 ;
         if (newval != oldval)
         {
            *vs->val_ptr = newval;
            LGadDrawBox(vb,NULL); // redraw with new selection
         }
         retval = TRUE;
      }
   }
   return(retval);
}

bool ScaleMouseHandler(short x, short y, short action, LGadBox *vb)
{
   LGadScale *vs;
   Rect *r = (BOX_RECT(vb));
   bool retval = TRUE;
   bool use_scale;
   short tw,th; // temp width and height

   vs = (LGadScale *)vb;
   if (vs->scale_flags & SCALE_READONLY)
      return(TRUE);

   // compensate for internal drawelement stuff
   ElementOffsetAlternate(&BUTTON_DRAWELEM(vs),&tw,&th);

   if (vs->scale_flags & SCALE_TITLELEFT)
   {
      short ew,eh;
      if (vs->internal_w == 0)
         ElementSize(&BUTTON_DRAWELEM(vs),&ew,&eh);
      else
         ew = vs->internal_w;
      if (x > ew)
         use_scale = TRUE;
      else
         use_scale = FALSE;
   }
   else
   {
      if (y > r->lr.y - (vs->dot_h + vs->dot_margin) - th)
         use_scale = TRUE;
      else
         use_scale = FALSE;
   }
   if ((action & (MOUSE_LDOWN|MOUSE_RDOWN)) && use_scale)
   {
      vs->down = TRUE;
      uiSetMouseMotionPolling(TRUE);
      ScaleMotionHandler(x,y,vb);
      uiGrabFocus(&VB(vs)->r,UI_EVENT_MOUSE);
   }
   if (action & (MOUSE_LUP|MOUSE_RUP))
   {
      if ((!vs->down) && !(vs->scale_flags & SCALE_NOTOGGLE))
         retval = ToggleMouseHandler(x,y,action,vb); // otherwise, act like a toggle
      else
      {
         if (vs->buttonfunc)
            retval = vs->buttonfunc(action, NULL, vb);
      }
      vs->down = FALSE;
      uiSetMouseMotionPolling(FALSE);
      uiReleaseFocus(&VB(vs)->r,UI_EVENT_MOUSE);
   }
   return(retval);
}

// We go through this wacky convolution of drawing a DrawElement callback that
// is our real draw callback so that the contents of the scale will wind up inside
// the whole combined border/internal paraphenalia
#pragma off(unreferenced)
void ScaleDrawCallback(DrawElement* elem, DrawElemState state)
{
   LGadScale *vs = (LGadScale *)elem->draw_data2;
   short x,y; // coords for drawing the pips
   ushort fl; // to hold original draw flags
   int i; // iterator
   ulong bcolor;
   DrawElement* draw = &BUTTON_DRAWELEM(vs);

   // Draw the draw element part (title, or whatever)
   fl = draw->draw_flags; // back up old value
   draw->draw_flags &= ~(DRAWFLAG_INTERNAL_BITS|DRAWFLAG_BORDER_BITS); // turn off inappropriate drawing stuff

   if (vs->scale_flags & SCALE_TITLELEFT)
   {
      short ew,eh;

      if (vs->internal_w == 0)
         ElementSize(draw, &ew, &eh);
      else
         ew = vs->internal_w;
      x = ew + vs->dot_margin;
      y = vs->dot_margin;
      ElementDraw(draw,dsNORMAL, 0, 0, x, grd_canvas->bm.h);
   }
   else
   {
      x = vs->dot_margin;
      y = grd_canvas->bm.h - (vs->dot_h + vs->dot_margin);
      ElementDraw(draw, dsNORMAL, 0, 0, grd_canvas->bm.w, y);
   }
   draw->draw_flags = fl; // restore to old val

   // background for the pips
   bcolor = get_color(VB(vs),StyleColorBG,draw->bcolor);

   if (bcolor != 0)
   {
      gr_set_fcolor(bcolor);
      ConfigSpew("scale",("setting bcolor to %d!\n",bcolor));
      gr_rect(x, y, grd_canvas->bm.w, grd_canvas->bm.h);
   }

   // Draw the cool pips
   for (i=0; i < vs->max_val; i += vs->increm)
   {
      if (*vs->val_ptr >= i)
         gr_set_fcolor(get_color(VB(vs),StyleColorBright,vs->lit_color));
      else
         gr_set_fcolor(get_color(VB(vs),StyleColorDim,vs->dim_color));
      gr_rect(x, y, x + vs->dot_w, y + vs->dot_h);
      x += vs->dot_w + vs->dot_margin;
   }
}

void ScaleDraw(void *data, LGadBox *vb)
{
   LGadScale *vs = (LGadScale *)vb;
   DrawElement de;

   ElementClear(&de);
   de.draw_type = DRAWTYPE_CALLBACK;
   de.draw_flags = BUTTON_DRAWELEM(vs).draw_flags;
   de.draw_data = (void *)ScaleDrawCallback;
   de.draw_data2 = (void *)vb;
   de.bcolor = BUTTON_DRAWELEM(vs).bcolor;

   // note assumption that we are in a canvas just right for us to draw into
   ElementDraw(&de,dsNORMAL,0,0,grd_canvas->bm.w,grd_canvas->bm.h);
}
#pragma on(unreferenced)
void LGadInitScale(LGadScale *vs)
{
   memset(vs,0,sizeof(LGadScale));
   LGadInitToggle((LGadToggle *)vs);
   ElementClear(&BUTTON_DRAWELEM(vs));
}

LGadScale *LGadCreateScale(LGadScale *vs, LGadRoot *vr, short x, short y, short w, short h, char paltype)
{
   short numdots; // number of scale dots
   if (vs == NULL)
   {
      vs = (LGadScale *)Malloc(sizeof(LGadScale));
      LGadInitScale(vs);
   }
   else
   {
      if (VB(vs)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateScale: vs %x is already active!\n",vs));
         return(NULL);
      }
   }

   if (vs == NULL)
   {
      Warning(("LGadCreateScale: vs is NULL after Malloc!\n"));
      return(NULL);
   }
   if (vs->max_val % vs->increm)
      Warning(("LGadCreateScale: %d not a mult of %d!\n",vs->max_val,vs->increm));


   // Wire up data2 to variable
   switch(BUTTON_DRAWELEM(vs).draw_type)
   {
      case DRAWTYPE_VAR:
      case DRAWTYPE_VARSTRING:
      case DRAWTYPE_VARTEXTREF:
      case DRAWTYPE_VARRES:
      case DRAWTYPE_FORMAT:
      case DRAWTYPE_FORMATREF:
         BUTTON_DRAWELEM(vs).draw_data2 = vs->val_ptr;
         break;
   }

   numdots = vs->max_val / vs->increm;
   if ((w <= 0) || (h <= 0))
   {
      short elemw,elemh; // draw element sizes, need to be separate from w and h so that we can
                         // know which of w and h to override at the end.
      short scalew; // width of the scale part
      short tw, th; // temp for ht and wd

      elemw=w;
      elemh=h;
      LGadComputeVarSize((LGadToggle *)vs,&elemw,&elemh);
      if (vs->internal_w != 0)
         elemw = vs->internal_w;

      // see if the scale-specific elements require more width
      scalew = vs->dot_margin + ((vs->dot_margin + vs->dot_w) * numdots);
      // take into account border sizing and the like
      ElementExtrasSize(&BUTTON_DRAWELEM(vs), &tw, &th);
      if (vs->scale_flags & SCALE_TITLELEFT)
      {
         elemw = elemw + scalew + tw;
      }
      else
      {
         scalew = scalew + tw;
         if (scalew > elemw)
            elemw = scalew;
      }

      // add scale-specific height
      elemh = elemh + vs->dot_h + vs->dot_margin;

      // plug in computed vals
      if (w <= 0)
         w = elemw;
      if (h <= 0)
         h = elemh;
   }

   LGadCreateBox((LGadBox *)vs,vr,x,y,w,h,ScaleMouseHandler,NULL,ScaleDraw,paltype);
   LGadBoxMouseMotion((LGadBox *)vs,ScaleMotionHandler);
   return(vs);
}

LGadScale *LGadCreateScaleArgs(LGadScale *vs, LGadRoot *vr, short x, short y, short w, short h,
   DrawElement *draw, LGadButtonCallback bfunc, int *val_ptr, short max_val,
   short increm, short lit_color, short dim_color, short dot_w, short dot_h, short dot_margin,
   ushort scale_flags, short internal_w, char paltype)
{
   if (vs == NULL)
   {
      vs = (LGadScale *)Malloc(sizeof(LGadScale));
      LGadInitScale(vs);
   }
   else
   {
      if (VB(vs)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateScaleArgs: vs %x is already active!\n",vs));
         return(NULL);
      }
   }

   if (vs == NULL)
   {
      Warning(("LGadCreateScaleArgs: vs is NULL after Malloc!\n"));
      return(NULL);
   }

   vs->val_ptr = val_ptr;
   vs->max_val = max_val;
   vs->increm = increm;
   vs->lit_color = lit_color;
   vs->dim_color = dim_color;
   vs->dot_h = dot_h;
   vs->dot_w = dot_w;
   vs->dot_margin = dot_margin;
   vs->buttonfunc = bfunc;
   vs->internal_w = internal_w;
   vs->scale_flags = scale_flags;
   BUTTON_DRAWELEM(vs) = *draw;
   return(LGadCreateScale(vs,vr,x,y,w,h,paltype));
}

// ********** Menu **********

// Assumes the "menu" palette
// if we have editmenus in other palettes we need to provide alternatives
#define EDITMENU_ACTIVE_COLOR    0x52
#define EDITMENU_INACTIVE_COLOR  0x20
#define EDITMENU_READONLY_COLOR  lgad_pal_text[0]

#define SCROLL_THICK 4

void LGadInitMenu(LGadMenu *vm)
{
   memset(vm,0,sizeof(LGadMenu));
   LGadInitButton((LGadButton *)vm);
}

#define MENU_MARGIN  0

void LGadMenuSaveunder(LGadMenu *vm)
{
   if (vm->flags & MENU_SAVEUNDER_DISABLE)
      return;

   // This relies on not being a direct-to-screen gadget!!
   if (!(vm->flags & MENU_SAVEUNDER_TAKEN))
   {
      Rect r;
      GUIcompose c;

      // Mouse hide/show should be done by gui composer
      region_abs_rect(LGadBoxRegion(vm),BOX_RECT(VB(vm)),&r); 
      GUIsetup(&c,&r,ComposeFlagRead|ComposeFlagReadOnly|ComposeFlagSaveUnder,
               GUI_CANV_SCREENDEPTH);
      gr_get_bitmap(&(vm->saveunder),0,0);
      GUIdone(&c);
      vm->flags |= MENU_SAVEUNDER_TAKEN;
   }
}

void MenuElemSize(LGadMenu *vm, short *pw, short *ph)
{
   short ew,eh;
   ElementExtrasSize(&BUTTON_DRAWELEM(vm),&ew,&eh);
   if (vm->flags & MENU_HORIZONTAL)
   {
      *pw = (RectWidth(BOX_RECT(VB(vm))) - (2 * MENU_MARGIN) - ew) / vm->vis_elems;
      *ph = RectHeight(BOX_RECT(VB(vm))) - (2 * MENU_MARGIN) - eh;
   }
   else
   {
      *pw = RectWidth(BOX_RECT(VB(vm))) - (2 * MENU_MARGIN) - ew;
      *ph = (RectHeight(BOX_RECT(VB(vm))) - (2 * MENU_MARGIN) - eh) / vm->vis_elems;
   }
}

#pragma off(unreferenced)
void MenuDrawCallback(DrawElement* elem, DrawElemState state)
{
   int i; // iterator
   LGadMenu *vm = (LGadMenu *)elem->draw_data2;
   short x,y,w,h;
   int v; // effective visible items


   x = MENU_MARGIN;
   y = MENU_MARGIN;
   MenuElemSize(vm,&w,&h);

   v = vm->vis_elems;

   // scrollbars
   if (vm->flags & MENU_SCROLLBARS_USED)
   {
      real pct; // how far down are we
      short coord; // where to draw pip
      Rect *r = LGadBoxRect(vm);
      Point sz;
      ulong color = get_color_default(VB(vm),StyleColorText,lgad_pal_text[VB(vm)->paltype]);

      ElementExtrasSize(&BUTTON_DRAWELEM(vm), &sz.x, &sz.y);
      sz.x = RectWidth(r) - sz.x - (2 * MENU_MARGIN);
      sz.y = RectHeight(r) - sz.y - (2 * MENU_MARGIN);

      pct = real_div(real_make(vm->focus,0),real_make(vm->num_elems - vm->vis_elems,0));
      gr_set_fcolor(color);
      if (vm->flags & MENU_HORIZONTAL)
      {
         gr_box(x,y,x + sz.x, y + SCROLL_THICK);
         coord = x + real_int(pct * (sz.x - SCROLL_THICK));
         gr_rect(coord,y,coord+SCROLL_THICK,y+SCROLL_THICK);
         y = y + SCROLL_THICK + MENU_MARGIN;
      }
      else
      {
         gr_box(x,y,x + SCROLL_THICK, y + sz.y);
         coord = y + real_int(pct * (sz.y - SCROLL_THICK)); // leave room for the position indicatior
         gr_rect(x,coord,x+SCROLL_THICK,coord+SCROLL_THICK);
         x = x + SCROLL_THICK + MENU_MARGIN;
      }
   }

   // all that normal stuff
   for (i=vm->focus; i < vm->focus + v; i++)
   {
      DrawElemState state = dsNORMAL;
      if ((vm->active_slot == i) && ((vm->flags & MENU_MOVEKEYS) || vm->mouse_sel >= 0))
      {
         state = dsATTENTION;
         //         fc = vm->elems[i].fcolor;
         //         vm->elems[i].fcolor = EDITMENU_ACTIVE_COLOR;
      }
      if ((vm->cur_sel == i) && !(vm->flags & MENU_SELBOX))
      {
         state = dsDEPRESSED;
         //         vm->elems[i].fcolor = ~vm->elems[i].fcolor; // wacky!
      }
      ElementDraw(&(vm->elems[i]),state,x,y,w,h);
      if (vm->cur_sel == i)
      {
         if (vm->flags & MENU_SELBOX)
         {
            ulong color = get_color_default(VB(vm),StyleColorHilite,EDITMENU_ACTIVE_COLOR);
            gr_set_fcolor(color);
            gr_box(x,y,x+w,y+h);
            gr_set_fcolor(color);
            gr_rect(0,0,3,3); //umm, somehow this seems not so abstract - MAHK
         }
         else
         {
            //            vm->elems[i].fcolor = ~vm->elems[i].fcolor; // wacky!
            vm->cur_sel = -1;
         }
      }
      //      if ((vm->active_slot == i) && (vm->flags & MENU_MOVEKEYS))
      //         vm->elems[i].fcolor = fc;

      if (vm->flags & MENU_HORIZONTAL)
         x = x + w + MENU_MARGIN;
      else
         y = y + h + MENU_MARGIN;
   }
}
#pragma on(unreferenced)


static void do_deferred_menu_destroy(void* _menu)
{
   LGadMenu* menu = (LGadMenu*)_menu;
   LGadDestroyMenu(menu);
}

static void menu_undraw(LGadMenu* vm)
{
   Rect  r = *LGadBoxRect(vm);
   region_abs_rect(LGadBoxRegion(vm),&r,&r);
   if (vm->vmredraw)
   {
      vm->vmredraw(r.ul.x,r.ul.y,RectWidth(&r),RectHeight(&r));
      vm->vmredraw = NULL;
   }
   else if ((!(vm->flags & MENU_SAVEUNDER_DISABLE)) && (vm->flags & MENU_SAVEUNDER_TAKEN))
   {
      GUIcompose c;
      uiHideMouse(&r);
      GUIsetup(&c,&r,ComposeFlagSaveUnder,GUI_CANV_SCREENDEPTH);
      gr_bitmap(&(vm->saveunder),0,0);
      GUIdone(&c);
      uiShowMouse(&r);
      Free(vm->saveunder.bits);
      vm->flags &= ~MENU_SAVEUNDER_TAKEN;
   }
}

static void defer_menu_destroy(LGadMenu* menu)
{
   Rect r;
   Region* reg = LGadBoxRegion(menu);
   menu_undraw(menu);
   region_abs_rect(reg,reg->r,&r);
   region_set_invisible(reg,TRUE);
   region_expose(LGadBoxRegion(current_root),&r);
   uiDefer(do_deferred_menu_destroy,menu);
}

bool MenuMouseHandler(short x, short y, short action, LGadBox *vb)
{
   LGadMenu *vm = (LGadMenu *)vb; // the menu, duh
   Region* reg = LGadBoxRegion(vb);
   Rect* r = LGadBoxRect(vb);
   short w,h; // elem sizes
   short tw, th; // temp w and h
   int i; // iterator

   // relativize point to region coordinates
   x -= reg->abs_x - r->ul.x;
   y -= reg->abs_y - r->ul.y; 

   if ((action & (MOUSE_LUP|MOUSE_RUP)) ||
      ((action & (MOUSE_RDOWN|MOUSE_LDOWN)) &&
         (vm->flags & (MENU_MOUSEDOWNS|MENU_SCROLLBARS_USED))))
   {
      Point p = MakePoint(x,y); 
      // check for trivial reject
      if (!(vm->flags & MENU_SCROLL_TRACK) && (!RectTestPt(r,p))) // if mouse is not in menu at all, and we aren't tracking
      {
         // Either destroy the menu if it is dismissable, or else just ignore it
         if (vm->flags & MENU_OUTER_DISMISS)
         {
            vm->cur_sel = -1;
            if (vm->vmenuc(vm->cur_sel,vm))
               defer_menu_destroy(vm);
            return(TRUE);
         }
      }

      // general computations
      ElementOffset(&BUTTON_DRAWELEM(vm), &tw, &th);

      // compute coords relative to gadget
      x -=  r->ul.x + tw + MENU_MARGIN;
      y -=  r->ul.y + th + MENU_MARGIN;

      // set up some selection information
      vm->sel_info = 0;
      if (action & MOUSE_LUP)
         vm->sel_info |= MENU_SELINFO_LUP;
      if (action & MOUSE_RUP)
         vm->sel_info |= MENU_SELINFO_RUP;
      if (vm->flags & MENU_MOUSEDOWNS)
      {
         if (action & MOUSE_LDOWN)
            vm->sel_info |= MENU_SELINFO_LDN;
         if (action & MOUSE_RDOWN)
            vm->sel_info |= MENU_SELINFO_RDN;
      }
      if (vm->flags & MENU_SCROLLBARS_USED)
      {
         if (action & MOUSE_LDOWN)
         {
            bool in_slider = FALSE;
            ConfigSpew("menu_scroll",("x = %d, vs %d!\n",x,SCROLL_THICK));
            if (vm->flags & MENU_HORIZONTAL)
               in_slider = (y < SCROLL_THICK + MENU_MARGIN);
            else
               in_slider = (x < SCROLL_THICK + MENU_MARGIN);
            if (in_slider)
            {
               vm->flags |= MENU_SCROLL_TRACK;
               vm->sel_info &= ~MENU_SELINFO_LDN;
               uiSetMouseMotionPolling(TRUE);
               ConfigSpew("menu_scroll",("menu scroll tracking!\n"));
               return(TRUE);
            }
         }
         else if ((vm->flags & MENU_SCROLL_TRACK) && (action & MOUSE_LUP))
         {
            vm->flags &= ~MENU_SCROLL_TRACK;
            vm->sel_info &= ~MENU_SELINFO_LUP;
            uiSetMouseMotionPolling(FALSE);
            ConfigSpew("menu_scroll",("menu scroll released!\n"));
            return(TRUE);
         }
      }

      if ((vm->vmenuc) && vm->sel_info)
      {
         MenuElemSize(vm,&w,&h);

         // now just divvy up the remaining space!
         if (vm->flags & MENU_HORIZONTAL)
            i = x / (w + MENU_MARGIN);
         else
            i = y / (h + MENU_MARGIN);
         if (i >= vm->vis_elems)
            i = vm->vis_elems-1;

         vm->cur_sel = vm->focus + i;
         // check for special value of -1 to indicate that the event was grabbed
         // by the gadget itself
         if (i != -1)
         {
            if (vm->vmenuc(vm->cur_sel,vm))
               defer_menu_destroy(vm);
            else if (vm->flags & MENU_SELBOX)
               LGadDrawBox(VB(vm),0);
         }

         if (vm->active_slot != vm->cur_sel)
            vm->active_slot = vm->cur_sel;

          return(TRUE);
      }

      else if (vm->vmenuc == NULL)
         Warning(("MenuMouseHandler: vm->vmenuc is NULL!\n"));

   }
   if (vm->flags & MENU_GRAB_FOCUS)
      return(TRUE);
   else
      return(FALSE);
}


#pragma off(unreferenced)
bool MenuMotionHandler(short x, short y, LGadBox *vb)
{
   LGadMenu *vm = (LGadMenu *)vb;
   Rect*r = BOX_RECT(VB(vm)); // dims of menu
   Point p; // the click, made structure
   Point sz; // for compensating for extra size
   bool in_rect = RECT_TEST_PT(r,MakePoint(x,y));

   // if we're a transient menu, destroy when the mouse leaves the menu
   if (vm->flags & MENU_TRANSIENT)
   {
      if (!in_rect)  // is the mouse out of the menu?
      {
         vm->cur_sel = -1;
         if (vm->vmenuc(vm->cur_sel,vm))
            defer_menu_destroy(vm);
      }
      return TRUE;
   }
   else if (vm->flags & MENU_SCROLL_TRACK)
   {
      real pct; // what fraction down are we?
      Point p2;
      int new_f; // new focus point

      // figure out how far "down" the slider the mouse is
      ElementOffset(&BUTTON_DRAWELEM(vm), &p2.x, &p2.y);
      p.x = x - r->ul.x - p2.x;
      p.y = y - r->ul.y - p2.y;

      // now figure the total space available
      ElementExtrasSize(&BUTTON_DRAWELEM(vm), &sz.x, &sz.y);
      sz.x = RectWidth(r) - sz.x;
      sz.y = RectHeight(r) - sz.y;

      if (vm->flags & MENU_HORIZONTAL)
         pct = real_div(real_make(p.x,0),real_make(sz.x,0));
      else
      {
         pct = real_div(real_make(p.y,0),real_make(sz.y,0));
      }

      // now we set the focus to the correct proportion of possible focus space, which ranges from
      // zero to (num_elems - vis_elems) since all the space above vis_elems is the same focus value
      new_f = real_int(pct * (vm->num_elems - vm->vis_elems + 1));
      if (new_f > vm->num_elems - vm->vis_elems)
         new_f = vm->num_elems - vm->vis_elems;

      LGadFocusMenu(vm, new_f);
   }
   else if (in_rect)// change mouse_sel and maybe active_slot
   {
      int i;
      int new_sel;
      short w,h;
      // make local copies of x and y
      short rx,ry;
      short tw,th;

      ElementOffset(&BUTTON_DRAWELEM(vm), &tw, &th);
      // compute coords relative to gadget
      rx = x - r->ul.x - tw - MENU_MARGIN;
      ry = y - r->ul.y - th - MENU_MARGIN;

      MenuElemSize(vm,&w,&h);
      if (vm->flags & MENU_HORIZONTAL)
         i = rx / (w + MENU_MARGIN);
      else
         i = ry / (h + MENU_MARGIN);
      if (i >= vm->vis_elems)
         i = vm->vis_elems-1;

      new_sel = vm->focus + i;

      if (vm->mouse_sel != new_sel)
      {
         vm->active_slot = new_sel;
         vm->mouse_sel = new_sel;
         LGadDrawBox(VB(vm),NULL);
      }
      else
         vm->mouse_sel = new_sel;
   }
   else
   {
      bool redraw = vm->mouse_sel >= 0;
      vm->mouse_sel = -1;
      if (redraw)
         LGadDrawBox(VB(vm),NULL);
   }

   if (vm->flags & MENU_GRAB_FOCUS)
      return(TRUE);
   else
      return(FALSE);
}
#pragma on(unreferenced)




bool MenuKeyHandlerInternal(short keycode, LGadBox *vb,bool default_retval)
{
   LGadMenu *vm = (LGadMenu *)vb; // the menu, duh
   int code = keycode & ~(KB_FLAG_DOWN);
   int oldf; // previous focus, for appropriate PGUP/PGDN behavior

   switch(code)
   {
      case KEY_ESC:
         if (vm->flags & MENU_ESC) {
            vm->cur_sel = -1;
            if (vm->vmenuc(vm->cur_sel,vm))
               defer_menu_destroy(vm);
            return TRUE;
         }
         break;
      case KEY_HOME:
         if (vm->flags & MENU_MOVEKEYS)
         {
            vm->active_slot = 0;
            LGadFocusMenu(vm,0);
         }
         break;
      case KEY_END:
         if (vm->flags & MENU_MOVEKEYS)
         {
            LGadFocusMenu(vm,vm->num_elems - vm->vis_elems);
            vm->active_slot = vm->num_elems - 1;
         }
         break;
      case KEY_PGDN:
         // note for page up and down we shift by vis_elems - 1 to
         // maintain visual continuity
         if (vm->flags & MENU_MOVEKEYS)
         {
            oldf = vm->focus;
            LGadFocusMenu(vm,vm->focus + (vm->vis_elems - 1));
            if (vm->active_slot < vm->focus)
               vm->active_slot = vm->focus;
            if (vm->focus == oldf)
               vm->active_slot = vm->num_elems - 1;
         }
         break;
      case KEY_PGUP:
         if (vm->flags & MENU_MOVEKEYS)
         {
            oldf = vm->focus;
            LGadFocusMenu(vm,vm->focus - (vm->vis_elems - 1));
            if (vm->active_slot > vm->focus + vm->vis_elems - 1)
               vm->active_slot = vm->focus + vm->vis_elems -1;
            if (vm->focus == oldf)
               vm->active_slot = 0;
         }
         break;
      case KEY_TAB:
      case KEY_DOWN:
      case KEY_PAD_DOWN:
         if (vm->flags & MENU_MOVEKEYS)
         {
            vm->active_slot = vm->active_slot + 1;
            if (vm->active_slot >= vm->num_elems)
               vm->active_slot = vm->num_elems - 1;
            if (vm->active_slot - vm->focus >= vm->vis_elems)
               LGadFocusMenu(vm,vm->active_slot - vm->vis_elems + 1);
         }
         break;
      case KEY_PAD_UP:
      case KEY_UP:
      case KEY_TAB|KB_FLAG_SHIFT:
         if (vm->flags & MENU_MOVEKEYS)
         {
            vm->active_slot = vm->active_slot - 1;
            if (vm->active_slot < 0)
               vm->active_slot = 0;
            if (vm->active_slot < vm->focus)
               LGadFocusMenu(vm,vm->active_slot);
         }
         break;
      case KEY_ENTER:
         if (vm->flags & MENU_MOVEKEYS)
         {
            vm->cur_sel = vm->active_slot;
            if (vm->vmenuc(vm->cur_sel,vm))
               defer_menu_destroy(vm);
            return TRUE;
         }
         break;
      default:
         return default_retval;

   }
     // redraw since we probably change selections
   LGadDrawBox(VB(vm),NULL);
   return(TRUE);
}

bool MenuKeyHandler(short keycode, LGadBox *vb)
{
   LGadMenu* vm = (LGadMenu*)vb;
   return MenuKeyHandlerInternal(keycode,vb,(vm->flags & MENU_GRAB_FOCUS) != 0);
}

void LGadMenuComputeSize(short *w, short *h, short num_elems, short vis_elems, uint flags,
   DrawElement *elems, uint draw_flags, short extra_w, short extra_h)
{
   short maxw, maxh; // max w and h of any element
   short tw, th; // temp wid and ht
   int i; // iterator
   DrawElement d;

   maxw = maxh = 0;
   // autosizing
   if ((*w <= 0) || (*h <= 0))
   {
      for (i=0; i < num_elems; i++)
      {
         ElementSize(&(elems[i]),&tw,&th);
         if (tw > maxw)
            maxw = tw;
         if (th > maxh)
            maxh = th;
      }

      if (flags & MENU_HORIZONTAL)
      {
         if (*w <= 0)
            *w = ((maxw + MENU_MARGIN) * vis_elems) + (2 * abs(*w));
         if (*h <= 0)
            *h = maxh + MENU_MARGIN + (2 * abs(*h));
         if (flags & MENU_SCROLLBARS_USED)
            *h = *h + SCROLL_THICK + MENU_MARGIN;
      }
      else
      {
         if (*h <= 0)
            *h = ((maxh + MENU_MARGIN) * vis_elems) + (2 * abs(*h));
         if (*w <= 0)
            *w = maxw + MENU_MARGIN + (2 * abs(*w));
         if (flags & MENU_SCROLLBARS_USED)
            *w = *w + SCROLL_THICK + MENU_MARGIN;
      }
      // take into account size of overall draw elements
      d.draw_flags = draw_flags;
      ElementExtrasSize(&d,&tw,&th);
      *w = *w + extra_w + tw + MENU_MARGIN;
      *h = *h + extra_h + th + MENU_MARGIN;

   }
}

// Note the problem that this will only autosize to the size of those elements visible at creation
// time!!
void LGadMenuSetup(LGadMenu *vm, LGadRoot *vr, short x, short y, short w, short h, LGadKeyCallback vkc,
   short extra_w, short extra_h, char paltype)
{
   int i; // iterator

   // For "blank" DrawElems that have been partially initialized for convenience, fill in a useful color
   for (i=0; i < vm->num_elems; i++)
   {
      if ((vm->elems[i].bcolor == 0) && (vm->elems[i].fcolor == 0))
      {
         vm->elems[i].fcolor = lgad_pal_text[paltype];
         vm->elems[i].bcolor = lgad_pal_blacks[paltype];
      }
   }

   vm->cur_sel = -1;
   vm->active_slot = 0;
   vm->gadg.box.vmotc = NULL;

   // set up optional frobs for scrolling
   if (vm->vis_elems < vm->num_elems)
   {
      if (vm->flags & MENU_SCROLLBAR)
         vm->flags |= MENU_SCROLLBARS_USED;
   }

   LGadMenuComputeSize(&w, &h, vm->num_elems, vm->vis_elems, vm->flags, vm->elems, BUTTON_DRAWELEM(vm).draw_flags, extra_w, extra_h);

   LGadCreateBox(VB(vm), vr, x, y, w, h, MenuMouseHandler, vkc, ButtonDraw, paltype);
   LGadBoxMouseMotion(VB(vm), MenuMotionHandler);
   if (LGadAddOverlay(VB(vm)) == 0)
   {
      vm->flags |= MENU_SAVEUNDER_DISABLE;
   }
   else
   {
      // only set up the default redraw if we haven't installed an overlay
      if (vm->vmredraw == NULL)
      {
         // @TBD (toml 05-22-96): this bitmap should probably have a companion gr_close_bitmap(), however, since we know it's in system memory we're okay.
         uchar *p = Malloc(w*h*(grd_bpp/8)); 
         gr_init_bitmap(&(vm->saveunder),p,grd_visible_canvas->bm.type,0, w, h);
      }
   }
   // okay to draw now since we have done appropriate saveunder stuff
   LGadMenuSaveunder(vm);
   if (!(vm->flags & MENU_NOAUTODRAW))
      LGadDrawBox(VB(vm),NULL);
   if (vm->flags & MENU_GRAB_FOCUS)
   {
      uiGrabFocus(&VB(vm)->r,ALL_EVENTS);
   }

}

/* Creates a menu interface element which displays a list of DrawElements and lets the user choose one of them.
As is the standard for these gadgets, if the LGadMenu passed in is NULL, the memory for one will be allocated,
otherwise it will use that pointer as valid memory to build the menu out of.  The LGadMenu by default will use
the memory passed in in the elems parameter as the actual memory it wants for storing the DrawElements, so you
need to be prepared to leave those sitting around.
If w or h are negative then the interface will be auto-sized in that dimension, as per the usual.

If the MENU_TRANSIENT flag is set, then the menu will be destroyed as soon as the mouse moves out of the
menu area, or any up events come through.  Normally, the menu goes away when the selection callback is
triggered and returns TRUE.  A non-transient menu will survive clicks outside of the menu box.

The vmredraw will get called when the menu gets destroyed, and the things underneath it are supposed to
redraw.  If the redraw callback passed in is NULL then the LGadMenu will allocate enough memory to save
the graphics under the menu information and then blast it back out when the menu goes away.  If other
systems are actively doing things underneath the menu while it is up then do not use this feature!

draw_flags is used to determine overall formatting for the menu.  Primarily the BORDER
and INTERNAL bits are relevant to that.

It is important to note that menus, unlike many other forms of gadgets, automatically draw themselves when
they are created!
*/

LGadMenu *LGadCreateMenu(LGadMenu *vm, LGadRoot *vr, short x, short y, short w, short h, char paltype)
{
   LGadKeyCallback kh;

   if (vm == NULL)
   {
      vm = (LGadMenu *)Malloc(sizeof(LGadMenu));
      LGadInitMenu(vm);
      vm->flags |= MENU_ALLOC_BASE;
   }
   else
   {
      if (VB(vm)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateMenu: vm %x is already active!\n",vm));
         return(NULL);
      }
   }



   if (vm == NULL)
   {
      Warning(("LGadCreateMenu: vm is NULL after malloc!\n"));
      return(NULL);
   }

   {
      DrawElement* draw = &BUTTON_DRAWELEM(vm);
      draw->draw_type = DRAWTYPE_CALLBACK;
      draw->draw_data = (void *)MenuDrawCallback;
      draw->draw_data2 = (void *)((LGadBox *)vm);
      draw->bcolor = lgad_pal_blacks[paltype];
   }

   if (vm->flags & MENU_STDKEYS)
      kh = MenuKeyHandler;
   else
      kh = NULL;
   LGadMenuSetup(vm,vr,x,y,w,h,kh,0,0,paltype);

   return(vm);
}

/* If you set the MENU_ALLOC_ELEMS flag when you create the Menu, then it will automatically allocate
the memory for the DrawElements itself, and copy the information out of the DrawElements you passed it,
and then automatically deallocate that memory when the menu is done.  Beware fragmentation! */

LGadMenu *LGadCreateMenuArgs(LGadMenu *vm, LGadRoot *vr, short x, short y, short w,
	short h, short num_elems, short vis_elems, DrawElement *elems, LGadMenuCallback vmc,
	LGadMenuRedraw vmredraw, ushort flags, ushort draw_flags, DrawElement *inner, char paltype)
{
   if (vm == NULL)
   {
      vm = (LGadMenu *)Malloc(sizeof(LGadMenu));
      LGadInitMenu(vm);
      vm->flags = flags | MENU_ALLOC_BASE;
   }
   else
   {
      if (VB(vm)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateMenuArgs: vm %x is already active!\n",vm));
         return(NULL);
      }
      vm->flags = flags;
   }

   // massage some params
   if (vis_elems == 0)
      vis_elems = num_elems;


   if (vm == NULL)
   {
      Warning(("LGadCreateMenuArgs: vm is NULL after malloc!\n"));
      return(NULL);
   }


   // correct for non-scrolling
   if (num_elems < vis_elems)
      vis_elems = num_elems;
   vm->num_elems = num_elems;
   vm->vis_elems = vis_elems;
   vm->vmenuc = vmc;
   vm->vmredraw = vmredraw;
   BUTTON_DRAWELEM(vm).draw_flags = draw_flags;
   BUTTON_DRAWELEM(vm).inner = inner;
   vm->focus = 0;

   if (vm->flags & MENU_ALLOC_ELEMS)
   {
      int i; // iterator
      vm->elems = (DrawElement *)Malloc(sizeof(DrawElement) * num_elems);
      for (i=0; i < num_elems; i++)
         vm->elems[i] = elems[i];
   }
   else
   {

      if (elems == NULL)
      {
         Warning(("LGadCreateMenuArgs: elems is NULL but is not MENU_ALLOC_ELEMS!\n"));
         return(NULL);
      }

      vm->elems = elems;
   }

   return(LGadCreateMenu(vm,vr,x,y,w,h,paltype));
}

// Get rid of the menu, which deals with replacing underneath it.
int LGadDestroyMenu(LGadMenu *vm)
{
   bool freeme; // whether or not to free up the menu memory

   menu_undraw(vm);

   if (vm->flags & MENU_ALLOC_ELEMS)
   {
      Free(vm->elems);
   }
   if (vm->flags & MENU_GRAB_FOCUS)
   {
      uiReleaseFocus(&VB(vm)->r,ALL_EVENTS);
   }

   freeme = vm->flags & MENU_ALLOC_BASE;

   LGadDestroyBox((LGadBox *)vm,freeme);
   if (!freeme)
      memset(vm,0,sizeof(LGadMenu));
   return(0); // success!
}

int LGadFocusMenu(LGadMenu *vm, int new_f)
{
   int v; // actual interesting visible elements

   if (vm == NULL)
   {
      Warning(("LGadFocusMenu: vm is NULL!\n"));
      return(-1);
   }


   v = vm->vis_elems;

   if (new_f + v > vm->num_elems)
   {
//      Warning(("LGadFocusMenu: new focus %d too large (vis %d, num %d)\n",new_f,vm->vis_elems,vm->num_elems));
      new_f = vm->num_elems - v;
   }
   if (new_f < 0)
      new_f = 0;

   if (new_f != vm->focus)
   {
      vm->focus = new_f;
      LGadDrawBox(VB(vm),NULL);
   }
   return(0);
}


// ********** Edit Menu **********

void LGadEditMenuParseText(VarElem *tv)
{
   int base = 10; // what counting base
   int cap = 10; // what is the highest valid decimal
   int i; // iterator
   bool okay = TRUE; // valid string?
   int atoival = 0; // final value for atoi, whether it is by hand or with real atoi
   int newval; // temp value for individual digits
   int smax; // maximum valid value for signed
   uint umax; // maximum valid value for unsigned

   if (tv->flags & EDITFLAG_HEX)
   {
      base = 16;
      cap = 10;
   };  // advance past the 0x
   if (tv->flags & EDITFLAG_OCTAL) { base = 8; cap = 8; };
   if (tv->flags & EDITFLAG_UNSIGNED)
   {
      switch(tv->vtype)
      {
         case EDITTYPE_INT: umax = UINT_MAX; break;
         case EDITTYPE_SHORT: umax = USHRT_MAX; break;
         case EDITTYPE_CHAR: umax = UCHAR_MAX; break;
      }
   }
   else
   {
      switch(tv->vtype)
      {
         case EDITTYPE_INT: smax = INT_MAX; break;
         case EDITTYPE_SHORT: smax = SHRT_MAX; break;
         case EDITTYPE_CHAR: smax = SCHAR_MAX; break;
      }
   }

   if (base == 10)
      atoival = atoi(tv->edit);
   else
   {
      // validate the string
      for (i=0; i < strlen(tv->edit); i++)
      {
         tv->edit[i] = toupper(tv->edit[i]);
         switch(base)
         {
            case 16:
               if ((tv->edit[i] < '0') || ((tv->edit[i] > '9') && (tv->edit[i] < 'A')) || (tv->edit[i] > 'F'))
                  okay = FALSE;
               break;
            case 8:
               if ((tv->edit[i] < '0') || (tv->edit[i] > '7'))
                  okay = FALSE;
               break;
            case 2:
               if ((tv->edit[i] < '0') || (tv->edit [i] > '1'))
                  okay = FALSE;
               break;
         }
      }
      if (okay)
      {
         for (i=0; i < strlen(tv->edit); i++)
         {
            if ((tv->edit[i] - '0') < cap)
            {
               newval = pow(base,strlen(tv->edit)-1-i) * (tv->edit[i] - '0');
               atoival += newval;
            }
            else if ((base == 16) && (tv->edit[i] >= 'A'))
            {
               newval = pow(16,strlen(tv->edit)-1-i) * (tv->edit[i] - 'A' + 10);
               atoival += newval;
            }
         }
      }
   }

   // At this point, atoival is the real actual value of the string.  Now we need to poke it into
   // the VarElem value pointer.
   if (!(tv->flags & EDITFLAG_UNSIGNED))  // not unsigned, thus signed
   {
      // insure that atoival is in a valid range
      if (atoival > smax)
         atoival = atoival % smax;

      // slam in the pointer
      switch(tv->vtype)
      {
         case EDITTYPE_INT: *((int *)tv->vdata) = (int)atoival; break;
         case EDITTYPE_SHORT: *((short *)tv->vdata) = (short)atoival; break;
         case EDITTYPE_CHAR: *((char *)tv->vdata) = (char)atoival; break;
      }
   }
   else
   {
      uint uatoival; // unsigned at the last, for valid mod-ing and stuff

      uatoival = (uint)atoival;

      // insure that atoival is in a valid range
      if (uatoival > umax)
         uatoival = uatoival % umax;

      // slam in the pointer
      switch(tv->vtype)
      {
         case EDITTYPE_INT: *((uint *)tv->vdata) = (uint)atoival; break;
         case EDITTYPE_SHORT: *((ushort *)tv->vdata) = (ushort)atoival; break;
         case EDITTYPE_CHAR: *((uchar *)tv->vdata) = (uchar)atoival; break;
      }
   }
}

void LGadEditMenuProcess(LGadEditMenu *vem)
{
   int i; // iterator;
   VarElem *tv; // temp varelem
   float tmpf;

   for (i=0; i < vem->num_elems; i++)
   {
      tv = &vem->varlist[i];
      switch(tv->vtype)
      {
         case EDITTYPE_INT:
         case EDITTYPE_SHORT:
         case EDITTYPE_CHAR:
            LGadEditMenuParseText(tv);
            break;
         case EDITTYPE_STRING:
            strcpy((char *)tv->vdata,tv->edit);
            break;
         case EDITTYPE_FIX:
            *((fix *)(tv->vdata)) = fix_from_float(atof(tv->edit));
            break;
         case EDITTYPE_FLOAT:
            tmpf=atof(tv->edit);
            *((float *)(tv->vdata)) = tmpf;
            break;
         case EDITTYPE_TOGGLE:
            {
               VarElem togve;
               EditToggleInfo *etip = (EditToggleInfo *)tv->vdata;

               // okay, we've been keeping the value of the toggle in the edit field, so we want to parse
               // it by constructing a fake VarElem with the right info and then parse it!
               togve.vdata = (void *)etip->togval;
               togve.vtype = EDITTYPE_INT;
               togve.flags = tv->flags;
               strcpy(togve.edit,tv->edit);
               LGadEditMenuParseText(&togve);
            }
            break;
      }
   }
}

bool EditMenuSelectHandler(int which_sel,LGadMenu *vm)
{
   LGadEditMenu *vem = (LGadEditMenu *)vm;
   VarElem *vep;
   bool retval = TRUE; // return of internal callbacks

   vep = &vem->varlist[which_sel];
   if (vep->flags & EDITFLAG_READONLY)
      return(FALSE);

   switch(vep->vtype)
   {
      case EDITTYPE_CALLBACK:
         {
            LGadButtonCallback vbc = (LGadButtonCallback)vep->vdata;
            vbc(MOUSE_LUP,NULL,VB(vm));
         }
         break;
      case EDITTYPE_CLOSE:
      case EDITTYPE_APPLY:
         // process all the text to assign new values!
         LGadEditMenuProcess(vem);
         // fallthrough
      case EDITTYPE_CANCEL:
         if (vep->vdata != NULL)
         {
            LGadButtonCallback vbc = (LGadButtonCallback)vep->vdata;
            retval = vbc(MOUSE_LUP,NULL,VB(vm));
         }
         if (retval && (vep->vtype != EDITTYPE_APPLY)) // don't close if we're just applying, or the callback sez no
            return(TRUE); // this will cause the menu to destroy itself and since we haven't gone and reprocessed
                          // any of the fields, we are all set to be nondestructive, unless we've fallen through
                          // in which case that's the right thing to be doing.
         break;
      case EDITTYPE_TOGGLE:
         {
            int delta = 1; // which way are we incrementing?
            int trueval; // what is the actual value of the slot, parsed from text
            EditToggleInfo *etip = (EditToggleInfo *)vep->vdata;

            if (vem->sel_info & MENU_SELINFO_RUP)
               delta = -1;
            trueval = atoi(vep->edit);
            trueval = trueval + delta;
            if (trueval < 0)
               trueval = etip->max_val;
            if (trueval > etip->max_val)
               trueval = 0;
            lg_sprintf(vep->edit,"%d",trueval);
         }
         break;
   }
   LGadDrawBox(VB(vm),NULL);

   return(FALSE);
}

// WARNING:  This will recurse infinitely if you give it a menu with nothing but READONLY elements!
void EditMenuChangeActive(LGadEditMenu *vem, int direction)
{
   vem->active_slot = vem->active_slot + direction;
   if (vem->active_slot >= vem->num_elems)
      vem->active_slot = 0;
   if (vem->active_slot < 0)
      vem->active_slot = vem->num_elems - 1;
   if (vem->varlist[vem->active_slot].flags & EDITFLAG_READONLY)
      EditMenuChangeActive(vem,direction);
}

bool EditMenuKeyHandler(short keycode, LGadBox *vb)
{
   LGadEditMenu *vem = (LGadEditMenu *)vb;
   VarElem *vep;
   bool retval; // for default menu key behavior
   int code = keycode & ~(KB_FLAG_DOWN | KB_FLAG_2ND);
   int n; // length of string
   int i,j; // iterator

   vep = &vem->varlist[vem->active_slot];
   n = strlen(vep->edit);

   switch (code)
   {
      case KEY_BS:
         if ((n > 0) && !(vep->flags & EDITFLAG_READONLY))
         {
            vep->edit[n-1] = '\0';
            vep->flags &= ~EDITFLAG_PRISTINE;
         }
         break;
      case KEY_TAB:
      case KEY_DOWN:
      case KEY_PAD_DOWN:
         EditMenuChangeActive(vem,1);
         break;
      case KEY_PAD_UP:
      case KEY_UP:
      case KEY_TAB|KB_FLAG_SHIFT:
         EditMenuChangeActive(vem,-1);
         break;
      case KEY_ENTER:
         if ((vep->vtype == EDITTYPE_CALLBACK) || (vep->vtype == EDITTYPE_CANCEL)
            || (vep->vtype == EDITTYPE_CLOSE) || (vep->vtype == EDITTYPE_APPLY)
            || (vep->vtype == EDITTYPE_TOGGLE))
         {
            if (EditMenuSelectHandler(vem->active_slot,(LGadMenu *)vem))
               defer_menu_destroy((LGadMenu*)vem);
            return(TRUE);  // since we don't want to do any more work given that we might have destroyed
                           // the menu with this action
         }
         else
         {
            // If the user hits return on a generic line, look for the first
            // destructive button and use that.
            // Look starting from current selection, then wrap around
            for (j=0; j < vem->num_elems; j++)
            {
               i = (j + vem->cur_sel) % vem->num_elems;
               if ((vem->varlist[i].vtype == EDITTYPE_CANCEL) || (vem->varlist[i].vtype == EDITTYPE_CLOSE))
               {
                  if (EditMenuSelectHandler(i,(LGadMenu *)vem))
                     defer_menu_destroy((LGadMenu *)vem);
                  return(TRUE);
               }
            }
         }
         break;
      case ('v'|KB_FLAG_CTRL):
         if (lgad_paste_func)
         {
            strncat(vep->edit,lgad_paste_func(),VARELEM_STRING_LEN - strlen(vep->edit));
            vep->flags &= ~EDITFLAG_PRISTINE;
         }
         break;
      case ('x'|KB_FLAG_CTRL):
         strcpy(vep->edit,"");
         vep->flags &= ~EDITFLAG_PRISTINE;
         break;
      default:
         retval = MenuKeyHandlerInternal(keycode,vb,FALSE);
         if (retval)
         {
            return(TRUE);
         }
         else
         {
            if (!(keycode & (KB_FLAG_CTRL|KB_FLAG_ALT)) && kb_isprint(keycode) && (n < VARELEM_STRING_LEN -1) && !(vep->flags & EDITFLAG_READONLY))
            {
               if ((vep->flags & EDITFLAG_INITCLEAR) && (vep->flags & EDITFLAG_PRISTINE))
               {
                  n = 0;
                  vep->flags &= ~EDITFLAG_PRISTINE;
               }
               vep->edit[n] = (keycode & 0xFF);
               vep->edit[n+1] = '\0';
            }
            else
               return(FALSE);
         }
         break;
   }

   LGadDrawBox(vb,NULL);
   return(TRUE);
}

#pragma off(unreferenced)
void EditMenuDrawCallback(DrawElement* elem, DrawElemState state)
{
   int i; // iterator
   LGadEditMenu *vm = (LGadEditMenu *)elem->draw_data2;
   short ew,eh;
   short x,y,w;
   DrawElement d;
   ushort fl; // draw flags storage
   int temp_val; // temporary storage value;


   x = MENU_MARGIN;
   y = MENU_MARGIN;
   w = grd_canvas->bm.w - (2 * MENU_MARGIN);

   for (i=0; i < vm->num_elems; i++)
   {
      // draw the label half
      if (vm->varlist[i].flags & EDITFLAG_READONLY)
         vm->elems[i].fcolor = EDITMENU_READONLY_COLOR;
      else if (vm->active_slot == i)
         vm->elems[i].fcolor = EDITMENU_ACTIVE_COLOR;
      else
         vm->elems[i].fcolor = EDITMENU_INACTIVE_COLOR;
      ElementSize(&(vm->elems[i]),&ew,&eh);
      switch (vm->varlist[i].vtype)
      {
         case EDITTYPE_INT:
         case EDITTYPE_SHORT:
         case EDITTYPE_CHAR:
         case EDITTYPE_STRING:
         case EDITTYPE_FLOAT:
         case EDITTYPE_FIX:
            // strip out border and internal types so that we can use them on the input part
            fl = vm->elems[i].draw_flags;
            vm->elems[i].draw_flags = fl & DRAWFLAG_FORMAT_BITS;
            ElementDraw(&(vm->elems[i]),dsNORMAL,x,y,ew,eh);
            vm->elems[i].draw_flags = fl;

            // draw the input half
            // setup the drawelem
            ElementClear(&d);
            d.draw_type = DRAWTYPE_TEXT;
            d.draw_flags = fl & ~DRAWFLAG_FORMAT_BITS;
            d.draw_flags |= FORMAT(DRAWFLAG_FORMAT_LEFT);
            if ((vm->varlist[i].flags & EDITFLAG_HEX) || (vm->varlist[i].flags & EDITFLAG_OCTAL))
            {
               char hextemp[VARELEM_STRING_LEN+2];
               if (vm->varlist[i].flags & EDITFLAG_HEX)
                  strcpy(hextemp,"0x");
               else
                  strcpy(hextemp,"0c");
               strcat(hextemp,vm->varlist[i].edit);
               d.draw_data = hextemp;
            }
            else
               d.draw_data = vm->varlist[i].edit;
            if (vm->varlist[i].flags & EDITFLAG_READONLY)
               d.fcolor = EDITMENU_READONLY_COLOR;
            else if (vm->active_slot == i)
               d.fcolor = EDITMENU_ACTIVE_COLOR;
            else
               d.fcolor = EDITMENU_INACTIVE_COLOR;
            d.bcolor = vm->elems[i].bcolor;

            // go!  Note that we use the height of the label part so the editable
            // part can't be smaller than the label part.
            ElementDraw(&d,dsNORMAL,x+ew+MENU_MARGIN,y,w-ew-MENU_MARGIN,eh);
            break;
         case EDITTYPE_CALLBACK:
         case EDITTYPE_CLOSE:
         case EDITTYPE_CANCEL:
         case EDITTYPE_APPLY:
         case EDITTYPE_TOGGLE:
            if (vm->varlist[i].vtype == EDITTYPE_TOGGLE)
            {
               temp_val = atoi(vm->varlist[i].edit);
               vm->elems[i].draw_data2 = (void *)&temp_val;
               ElementDraw(&(vm->elems[i]),dsNORMAL,x,y,w,eh);
               vm->elems[i].draw_data2 = NULL;
            }
            else
               ElementDraw(&(vm->elems[i]),dsNORMAL,x,y,w,eh);
            break;
      }

      y = y + eh + MENU_MARGIN;
   }
}
#pragma on(unreferenced)

void LGadEditMenuSetText(LGadEditMenu *vm)
{
   int i;
   VarElem *vep; // pointer for quick access of "current" VarElem
   char hextemp[VARELEM_STRING_LEN+2];
   fix tmpfix;
   int base;

   for (i=0; i < vm->num_elems; i++)
   {
      vep = &vm->varlist[i];
      vep->flags |= EDITFLAG_PRISTINE; // since it is in its initial state
      if (vep->flags & EDITFLAG_HEX)
      {
         base = 16;
      }
      else if (vep->flags & EDITFLAG_OCTAL)
         base = 8;
      else
         base = 10;
      switch(vep->vtype)
      {
         case EDITTYPE_INT:
            if (vep->flags & EDITFLAG_UNSIGNED)
               utoa(*((uint *)vep->vdata),vep->edit,base);
            else
               itoa(*((int *)vep->vdata),vep->edit,base);
            break;
         case EDITTYPE_SHORT:
            if (vep->flags & EDITFLAG_UNSIGNED)
               utoa(*((ushort *)vep->vdata),vep->edit,base);
            else
               itoa(*((short *)vep->vdata),vep->edit,base);
            break;
         case EDITTYPE_CHAR:
            if (vep->flags & EDITFLAG_UNSIGNED)
               utoa(*((uchar *)vep->vdata),vep->edit,base);
            else
               itoa(*((char *)vep->vdata),vep->edit,base);
            break;
         case EDITTYPE_STRING:
            strcpy(vep->edit,(char *)vep->vdata);
            break;
         case EDITTYPE_FIX:
            lg_sprintf(vep->edit,"%q",*((fix *)vep->vdata));
            break;
         case EDITTYPE_FLOAT:
            tmpfix=fix_from_float(*((float *)vep->vdata));
            lg_sprintf(vep->edit,"%q",tmpfix);
//            lg_sprintf(vep->edit,"%f",*((float *)vep->vdata));
            break;
         case EDITTYPE_CALLBACK:
         case EDITTYPE_CLOSE:
         case EDITTYPE_CANCEL:
            strcpy(vep->edit,"");
            break;
         case EDITTYPE_TOGGLE:
            {
               EditToggleInfo *etip = (EditToggleInfo *)vep->vdata;
               lg_sprintf(vep->edit,"%d",*(etip->togval));
               vm->elems[i].draw_data2 = NULL;
            }
            break;
      }
      if ((vep->flags & EDITFLAG_HEX) || (vep->flags & EDITFLAG_OCTAL))
      {
         strcpy(hextemp,vep->edit);
         if (vep->flags & EDITFLAG_HEX)
            strcpy(vep->edit,"0x");
         else
            strcpy(vep->edit,"0c");
         strcat(vep->edit,hextemp);
      }
      if ((vep->flags & EDITFLAG_HEX) || (vep->flags & EDITFLAG_OCTAL))
      {
         strcpy(vep->edit,hextemp);
      }
   }
}
/* A LGadEditMenu is a lot like a normal LGadMenu, but with a couple of critical changes.  The
primary one is that each line on the menu is both a DrawElement but also a VarElem, which keeps
track of a value and allows you to change it.  The last two elements in most LGadEditMenus should
probably be of EDITTYPE_CLOSE and EDITTYPE_CANCEL so that the data can be processed, or changes aborted. */

void LGadInitEditMenu(LGadEditMenu *vm)
{
   memset(vm,0,sizeof(LGadEditMenu));
   LGadInitMenu((LGadMenu *)vm);
}

LGadEditMenu *LGadCreateEditMenu(LGadEditMenu *vm, LGadRoot *vr, short x, short y, short w, short h, char paltype)
{
   int i; // iterator
   VarElem *vep; // pointer for quick access of "current" VarElem
   short tw,th; // width and height temp for params
   short max_w; // biggest thing thus far
   DrawElement d; // temp for computing width

   if (vm == NULL)
   {
      vm = (LGadEditMenu *)Malloc(sizeof(LGadEditMenu));
      LGadInitEditMenu(vm);
      vm->flags |= MENU_ALLOC_BASE;
   }
   else
   {
      if (VB(vm)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateEditMenu: vm %x is already active!\n",vm));
         return(NULL);
      }
   }

   if (vm == NULL)
   {
      Warning(("LGadCreateEditMenu: vm is NULL after malloc!\n"));
      return(NULL);
   }

   vm->flags |= MENU_GRAB_FOCUS;
   {
      DrawElement* draw = &BUTTON_DRAWELEM(vm);
      draw->draw_type = DRAWTYPE_CALLBACK;
      draw->draw_data = (void *)EditMenuDrawCallback;
      draw->draw_data2 = (void *)((LGadBox *)vm);
      draw->bcolor = lgad_pal_blacks[paltype];
   }
   vm->vmenuc = EditMenuSelectHandler;

   // Set up initial text in the editable part
   // and get size estimations, etc.
   max_w = 0;
   LGadEditMenuSetText(vm);
   for (i=0; i < vm->num_elems; i++)
   {
      vep = &vm->varlist[i];
      if (w <= 0)
      {
         ElementClear(&d);
         d.draw_type = DRAWTYPE_TEXT;
         d.draw_data = vm->varlist[i].edit;
         ElementSize(&d,&tw,&th);
         if (tw > max_w)
            max_w = tw;
      }
   }

   LGadMenuSetup((LGadMenu *)vm,vr,x,y,w,h,EditMenuKeyHandler,(max_w + MENU_MARGIN),0,paltype);

   return(vm);
}

LGadEditMenu *LGadCreateEditMenuArgs(LGadEditMenu *vm, LGadRoot *vr, short x, short y, short w,
	short h, short num_elems, short vis_elems, DrawElement *elems, ushort flags, ushort draw_flags,
	DrawElement *inner, VarElem *varlist, char paltype)
{
   if (vm == NULL)
   {
      vm = (LGadEditMenu *)Malloc(sizeof(LGadEditMenu));
      LGadInitEditMenu(vm);
      vm->flags = flags | MENU_ALLOC_BASE;
   }
   else
   {
      if (VB(vm)->box_flags & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateEditMenuArgs: vm %x is already active!\n",vm));
         return(NULL);
      }
      vm->flags = flags;
   }
   if (vm == NULL)
   {
      Warning(("LGadCreateEditMenuArgs: vm is NULL after malloc!\n"));
      return(NULL);
   }

   // massage some params
   if (vis_elems == 0)
      vis_elems = num_elems;
   if (vis_elems < num_elems)
   {
      Warning(("LGadCreateEditMenuArgs: vis_elems %d is smaller than num_elems %d!\n",vis_elems,num_elems));
      vis_elems = num_elems;
   }

   vm->vis_elems = vis_elems;
   vm->num_elems = num_elems;
   BUTTON_DRAWELEM(vm).draw_flags = draw_flags;
   BUTTON_DRAWELEM(vm).inner = inner;

   if (vm->flags & MENU_ALLOC_ELEMS)
   {
      int i; // iterator
      vm->elems = (DrawElement *)Malloc(sizeof(DrawElement) * num_elems);
      vm->varlist = (VarElem *)Malloc(sizeof(VarElem) * num_elems);
      for (i=0; i < num_elems; i++)
      {
         vm->elems[i] = elems[i];
         vm->varlist[i] = varlist[i];
      }
   }
   else
   {
      if (elems == NULL)
      {
         Warning(("LGadCreateEditMenuArgs: elems is NULL but is not MENU_ALLOC_ELEMS!\n"));
         return(NULL);
      }
      vm->elems = elems;
      vm->varlist = varlist;
   }

   return(LGadCreateEditMenu(vm,vr,x,y,w,h,paltype));
}


// ********** Infrastructure **********

#define NUM_HOTKEYS  70
extern void DrawElementInit(void);


#pragma off(unreferenced)
int LGadInit(void)
{
   hotkey_init(NUM_HOTKEYS);
   DrawElementInit();

   ui_dest_canvas = UtilScreen(UTIL_NONE);

   return(0);
}

int LGadTerm(void)
{
   hotkey_shutdown();

   return(0);
}

void LGadSimplePoll(void)
{
   uiPoll();
}

int LGadFrame(void)
{
   LGadRemoveUnusedOverlays();
   if (normal_input)
   {
      if (lgad_recorder_func != NULL)
         lgad_recorder_func();
      else
         LGadSimplePoll();
   }
   return(0);
}

#pragma on(unreferenced)

