// $Header: x:/prj/tech/libsrc/dev2d/RCS/bitmap.c 1.10 1998/03/26 11:49:52 KEVIN Exp $

#include <string.h>

#include <dbg.h>
#include <grd.h>
#include <bitmap.h>
#include <grmalloc.h>
#include <valloc.h>

uint gr_bm_type_to_flat(uint type)
{
   switch (type) {
   case BMT_RSD8:
   case BMT_TLUC8:
   case BMT_BANK8:
   case BMT_MODEX:
      type = BMT_FLAT8; break;
   case BMT_BANK16:
      type = BMT_FLAT16; break;
   case BMT_BANK24:
      type = BMT_FLAT24; break;
   }
   return type;
}

uchar gdd_pal_index = 0;

static void set_pow2_flag(grs_bitmap *bm)
{
   if ((bm->row==(1<<bm->wlog))&&
       (((bm->h-1)&bm->h)==0)&&
       (((bm->w-1)&bm->w)==0))
      bm->flags |= BMF_POW2;
   else
      bm->flags &= ~BMF_POW2;
}

/* initialize a new bitmap structure. set bits, type, flags, w, and h from
   arguments. set align to 0 and calculate row from width depending on what
   type of bitmap. */
void gr_init_bitmap (grs_bitmap *bm, uchar *p, uint type, uint flags,
                 int w, int h)
{
   int row;
   int v;

   /* generate flat bitmap of appropriate bitdepth to current mode. */
   if (type == BMT_DEVICE_VIDMEM_TO_FLAT)
      type = gr_bm_type_to_flat(grd_screen_bmt);

   /* calculate row from type and w. */
   switch (type) {
   case BMT_BANK8:
   case BMT_BANK16:
   case BMT_BANK24:
   case BMT_MODEX:
   case BMT_DEVICE_VIDMEM:
      flags |= BMF_DEVICE_VIDMEM;
      row = grd_screen_row;
      type = grd_screen_bmt;
      break;
   case BMT_MONO: row = (w+7)/8; break;
   case BMT_FLAT8:
   case BMT_TLUC8: row = w; break;
   case BMT_FLAT16: row = 2*w; break;
   case BMT_FLAT24: row = 3*w; break;
   case BMT_RSD8: row = 0; break;
   default:
      Warning(("gr_init_bm(): unsupported type\n"));
      row = 0;
      break;
   }

   bm->bits = p;
   bm->type = type;
   bm->flags = flags;
   if (type == BMT_FLAT8)
      bm->align = gdd_pal_index;
   else
      bm->align = 0;
   bm->w = w;
   bm->h = h;
   bm->row = row;
   bm->wlog = bm->hlog = 0;

   for (v=2; v<=row; v+=v)
      bm->wlog++;
   for (v=2; v<=h; v+=v)
      bm->hlog++;

   set_pow2_flag(bm);

   if (vMonitor(&bm->bits))
      bm->flags |= BMF_RELOCATABLE;
}

void gr_close_bitmap (grs_bitmap *bm)
{
   // Don't do anything if the 2D has already closed down
   if (gdd_active == 0)
      return;

   if (bm->flags&BMF_RELOCATABLE)
      vUnmonitor(&bm->bits);
}

void gr_init_sub_bitmap_unsafe (grs_bitmap *sbm, grs_bitmap *dbm, int x, int y,
                     int w, int h)
{
   memcpy (dbm, sbm, sizeof (*sbm));
   dbm->w = w; dbm->h = h;

   dbm->bits += y*dbm->row;
   switch (sbm->type) {
   case BMT_MONO:
      x += dbm->align;
      dbm->bits += x>>3;
      dbm->align = x&7;
      break;
   case BMT_MODEX:
      x += dbm->align;
      dbm->bits += x>>2;
      dbm->align = x&3;
      break;
   case BMT_FLAT8:
   case BMT_BANK8:
   case BMT_TLUC8:
      dbm->bits += x;
      break;
   case BMT_FLAT16:
   case BMT_BANK16:
      dbm->bits += 2*x;
      break;
   case BMT_FLAT24:
   case BMT_BANK24:
      dbm->bits += 3*x;
      break;
   case BMT_RSD8:
   default: break;
   }

   set_pow2_flag(dbm);

   // Ok, so we need to check to see if a particular bitmap is relocatable
   // If so, then its child is also and we need to monitor the child too.
   // We also know that if the parent is relocatable, then the SetMonitor
   // will work (since it worked for the parent) so we don't have to test
   // return value.
   if (sbm->flags & BMF_RELOCATABLE) {
      vMonitor(&dbm->bits);
      dbm->flags |= BMF_RELOCATABLE;
   }
}

/* set up a new bitmap structure to be a subsection of an existing bitmap.
   sbm is source bm, dbm destination. (0,0) of dbm maps to (x,y) of sbm,
   and dbm is w x h in size. */
void gr_init_sub_bitmap (grs_bitmap *sbm, grs_bitmap *dbm, int x, int y,
                     int w, int h)
{
   Assert((x>=0)&&(y>=0),
      ("gr_init_sub_bitmap(): upper left corner outside of source bitmap!\n%s",
       "If you really want to do this, use gr_init_sub_bitmap_unsafe().\n"));
   Assert((x+w<=sbm->w)&&(y+h<=sbm->h),
      ("gr_init_sub_bitmap(): lower right corner outside of source bitmap!\n%s",
       "If you really want to do this, use gr_init_sub_bitmap_unsafe().\n"));
   gr_init_sub_bitmap_unsafe(sbm, dbm, x, y, w, h);
}

static grs_bitmap tmp_bm;     /* temporary space for bitmap init */
/* allocate memory for a bitmap structure and the data for a bitmap of
   the specified type and flags of size w x h.  returns a pointer to the
   new bitmap structure.  the returned pointer can be freed in order to
   free both the structure and data memory. */
grs_bitmap *gr_alloc_bitmap (uint type, uint flags, int w, int h)
{
   grs_bitmap *bm;                  /* pointer to allocated buffer */
   int size=sizeof(tmp_bm);

   gr_init_bitmap(&tmp_bm, NULL, type, flags, w, h);
   if (type!=BMT_DEVICE_VIDMEM)
      size += tmp_bm.row*tmp_bm.h;

   bm=(grs_bitmap *)gr_malloc(size);

   if (bm != NULL) {
      memcpy(bm, &tmp_bm, sizeof(tmp_bm));
      if (type == BMT_DEVICE_VIDMEM) {
         if (vAlloc(&bm->bits, w, h))
            bm->flags |= BMF_RELOCATABLE;
      } else {
         bm->bits = ((uchar *)bm) + sizeof(tmp_bm);
      }
   }
   gr_close_bitmap(&tmp_bm);
   return bm;
}
