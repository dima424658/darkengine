// $Header: x:/prj/tech/libsrc/dev2d/RCS/canvas.c 1.16 1998/04/28 13:45:45 KEVIN Exp $
// Canvas handling routines.
//
// This file is part of the dev2d library.

#include <string.h>

#include <dbg.h>

#include <bitmap.h>
#include <context.h>
#include <fill.h>
#include <grd.h>
#include <grmalloc.h>
#include <valloc.h>

#define MAX_CB 8
typedef struct cb_entry_struct {
   void (*callback)(grs_canvas *c);
   int id;
   struct cb_entry_struct *next;
} cb_entry;

static cb_entry cb_array[MAX_CB]=
{
   {NULL, -1, NULL},   {NULL, -1, NULL},   {NULL, -1, NULL},   {NULL, -1, NULL},
   {NULL, -1, NULL},   {NULL, -1, NULL},   {NULL, -1, NULL},   {NULL, -1, NULL}
};
static int num_cb = 0;
static cb_entry *cb_list=NULL;
static int cb_next_id = 0;

int gr_install_set_canvas_callback(void (*callback)(grs_canvas *c))
{
   cb_entry *pcbe;
   cb_entry **ppcbe;
   if (num_cb >= MAX_CB) {
      Warning(("gr_install_set_canvas_callback():Too many callbacks installed.\n"));
      return -1;
   }
   pcbe = cb_array;
   while (pcbe->id != -1)
      pcbe++;
   pcbe->next = NULL;
   pcbe->id = cb_next_id;
   pcbe->callback = callback;

   ppcbe = &cb_list;
   while (*ppcbe!=NULL)
      ppcbe = &((*ppcbe)->next);
   *ppcbe = pcbe;

   num_cb++;
   return cb_next_id++;
}

void gr_uninstall_set_canvas_callback(int cb_id)
{
   cb_entry **ppcbe = &cb_list;
   while (*ppcbe!=NULL)
      if ((*ppcbe)->id == cb_id) {
         (*ppcbe)->id = -1;
         *ppcbe = (*ppcbe)->next;
         num_cb--;
         return;
      }
   Warning(("gr_uninstall_set_canvas_callback():Couldn't find callback with specified id.\n"));
}

/* set current canvas to c. select driver_func from type of bitmap
   attached to canvas. */
void gr_set_canvas (grs_canvas *c)
{
   cb_entry *cbe;

   grd_canvas = c;

   if (c->bm.flags&BMF_DEV_MUST_BLIT) {
      gdd_canvas_table = gdd_dispdev_canvas_table;
   } else {
      switch (c->bm.type) {
      case BMT_FLAT8:
         gdd_canvas_table = gdd_flat8_canvas_table;
         break;
      case BMT_FLAT16:
         gdd_canvas_table = gdd_flat16_canvas_table;
         break;
      case BMT_FLAT24:
         gdd_canvas_table = gdd_flat24_canvas_table;
         break;
      case BMT_BANK8:
         gdd_canvas_table = gdd_bank8_canvas_table;
         break;
      case BMT_BANK16:
         gdd_canvas_table = gdd_bank16_canvas_table;
         break;
      case BMT_BANK24:
         gdd_canvas_table = gdd_bank24_canvas_table;
         break;
      case BMT_MODEX:
         gdd_canvas_table = gdd_modex_canvas_table;
         break;
      default:
         Warning(("gr_set_canvas(): unsupported canvas bitmap type."));
      }
   }

   cbe = cb_list;
   while (cbe!=NULL) {
      cbe->callback(c);
      cbe = cbe->next;
   }
}

#define CANVAS_STACKSIZE 16
grs_canvas *gdd_canvas_stack[CANVAS_STACKSIZE];
int gdd_canvas_stackp = 0;

/* push current canvas onto canvas stack and make passed in canvas active.
   returns 0 if stack is ok, -1 if there is an overflow. */
int gr_push_canvas (grs_canvas *c)
{
   if (gdd_canvas_stackp >= CANVAS_STACKSIZE) {
      Error(1, "gr_push_canvas(): CANVAS_STACKSIZE exceeded!\n");
      return -1;
   }
   gdd_canvas_stack[gdd_canvas_stackp++] = grd_canvas;
   gr_set_canvas (c);
   return 0;
}

/* pop last canvas off of stack and make it active.  return it, or NULL if
   there is an underflow. */
grs_canvas *gr_pop_canvas (void)
{
   grs_canvas *c;
   if (gdd_canvas_stackp <= 0) {
      Error(1, "gr_pop_canvas(): top of canvas stack already reached!\n");
      return NULL;
   }
   c = gdd_canvas_stack[--gdd_canvas_stackp];
   gr_set_canvas (c);
   return c;
}

void gr_alloc_ytab(grs_canvas *c)
{
   int i, n, row, offset, *ytab;

   n = c->gc.safe_clip.i.bot - c->gc.safe_clip.i.top;
   row = c->bm.row;
   ytab = (int *)gr_malloc(n * sizeof(int));
   offset = c->gc.safe_clip.i.top * row;
   for (i=0; i<n; i++) {
      ytab[i] = offset;
      offset += row;
   }
   c->ytab = &ytab[-c->gc.safe_clip.i.top];
}

void gr_free_ytab(grs_canvas *c)
{
   if (c->ytab == NULL)
      return;

   gr_free(&c->ytab[c->gc.safe_clip.i.top]);
   c->ytab = NULL;
}

void gr_init_canvas (grs_canvas *c, uchar *p, uint type, int w, int h)
{
   gr_init_bitmap (&c->bm, p, type, 0, w, h);
   gr_init_gc (c);
   c->ytab = NULL;
}

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
void gr_init_sub_canvas (grs_canvas *sc, grs_canvas *dc, int x, int y,
                         int w, int h)
{
   grs_clip* safe = &sc->gc.safe_clip;
   gr_init_sub_bitmap_unsafe (&sc->bm, &dc->bm, x, y, w, h);
   gr_init_gc (dc);
   dc->gc.fill_type = sc->gc.fill_type;
   if (dc->gc.fill_type > FILL_TYPES) {
      Warning(("Canvas fill type out of range!\n"));
      dc->gc.fill_type = FILL_NORM;
   }
   dc->gc.fill_parm = sc->gc.fill_parm;
   
   // inherit the safe cliprect from the parent canvas
   gr_init_fix_clip(&dc->gc.safe_clip,
                    safe->f.left - fix_make(x,0),
                    safe->f.top  - fix_make(y,0),
                    safe->f.right - fix_make(x,0),
                    safe->f.bot - fix_make(y,0));
   // now set the cliprect safely
   gr_safe_cset_cliprect(dc,0,0,w,h);
   
   dc->ytab = NULL;
}


void gr_make_canvas (grs_bitmap *bm, grs_canvas *c)
{
   memcpy(&(c->bm), bm, sizeof(*bm));

   // If the bits are relocatable, need to monitor the canvas bits as well.
   if (bm->flags&BMF_RELOCATABLE)
      vMonitor(&c->bm.bits);

   gr_init_gc(c);
   c->ytab = NULL;
}

grs_canvas *gr_alloc_canvas (uint type, int w, int h)
{
   grs_canvas *c;
   uchar **p;

   if ((c=(grs_canvas *)gr_malloc (sizeof (*c))) == NULL)
      return NULL;
   gr_init_canvas (c, NULL, type, w, h);
   p = &(c->bm.bits);
   if (c->bm.flags&BMF_DEVICE_VIDMEM) {
      if (vAlloc(p, w, h))
         c->bm.flags|=BMF_RELOCATABLE;
   } else {
      switch (c->bm.type) {
         case BMT_FLAT8:
            *p = (uchar *)gr_malloc (w*h); break;
         case BMT_FLAT16:
            *p = (uchar *)gr_malloc (2*w*h); break;
         case BMT_FLAT24:
            *p = (uchar *)gr_malloc (3*w*h); break;
         default:
            Warning(("gr_alloc_canvas(): unsupported bitmap type requested.\n"));
      }
   }

   return c;
}

void gr_close_canvas(grs_canvas *c)
{
   if (c->ytab != NULL)
      gr_free_ytab(c);
   gr_close_bitmap(&(c->bm));
}

void gr_free_canvas (grs_canvas *c)
{
   gr_close_canvas(c);

   if (c->bm.flags&BMF_DEVICE_VIDMEM)
      vFree (&c->bm.bits);
   else
      gr_free (c->bm.bits);
   gr_free (c);
}

grs_canvas *gr_alloc_sub_canvas (grs_canvas *c, int x, int y,
   int w, int h)
{
   grs_canvas *c_new;

   c_new = (grs_canvas *)gr_malloc (sizeof (*c_new));
   if (c_new != NULL)
      gr_init_sub_canvas (c, c_new, x, y, w, h);
   return c_new;
}

void gr_free_sub_canvas (grs_canvas *c)
{
   gr_close_canvas(c);
   gr_free (c);
}

void gr_cset_fix_cliprect(grs_canvas* c, fix l, fix t, fix r, fix b)
{
   grs_clip *safe = &c->gc.safe_clip;             
   grs_clip *clip = &c->gc.clip;                  

   clip->f.sten=NULL;
   clip->f.left =gr_max(safe->f.left ,l);           
   clip->f.right=gr_min(safe->f.right,r);           
   clip->f.top=  gr_max(safe->f.top  ,t);           
   clip->f.bot=  gr_min(safe->f.bot  ,b);           
}

void gr_safe_cset_fix_cliprect(grs_canvas* c, fix l, fix t, fix r, fix b)
{
   gr_cset_fix_cliprect(c, l, t, r, b);
}
