// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16f16.c 1.8 1997/02/17 14:48:45 KEVIN Exp $
// Routines for drawing flat16 bitmaps into a flat16 canvas.

#include <dbg.h>
#include <astring.h>

#include <grd.h>
#include <bitmap.h>
#include <context.h>
#include <f16il.h>

void flat16_flat16_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   void (*memcpy_func)(void *dst, void *src, int count);
   int w, h, srow, drow;
   ushort *src, *dst;

   w = 2*bm->w;
   h = bm->h;
   if ((w<=0)||(h<=0)) {
      Warning(("flat16_flat16_opaque_ubitmap() bitmap has w<=0 or h<=0!\n"));
      return;
   }

   srow = bm->row;
   src = (ushort *)bm->bits;
   drow = grd_bm.row;
   dst = (ushort *)(grd_bm.bits + drow*y + 2*x);

   if (bm->flags&BMF_DEVICE_VIDMEM) {
      if ((grd_bm.flags&BMF_DEVICE_VIDMEM) &&
          (((long )src)&3!=((long )dst)&3)) {
         memcpy_func = memcpy_by_byte;
      } else {
         memcpy_func = memcpy_align_src;
      }
   } else {
      if (grd_bm.flags&BMF_DEVICE_VIDMEM) {
         memcpy_func = memcpy_align_dst;
      } else {
         memcpy_func = memcpy_cache_dst;
      }
   }

   goto oloop_start;

   do {
      src = (ushort *)(((uchar *)src)+srow);
      dst = (ushort *)(((uchar *)dst)+drow);
oloop_start:
      memcpy_func(dst, src, w);
   } while (--h);

}

void flat16_flat16_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   int w, h, srow, drow;
   ushort *src, *dst;

   w = bm->w;
   h = bm->h;
   if ((w<=0)||(h<=0)) {
      Warning(("flat16_flat16_trans_ubitmap() bitmap has w<=0 or h<=0!\n"));
      return;
   }

   srow = bm->row;
   src = (ushort *)bm->bits;
   drow = grd_bm.row;
   dst = (ushort *)(grd_bm.bits + drow*y + 2*x);

   goto tloop_start;

   do {
      src = (ushort *)(((uchar *)src)+srow);
      dst = (ushort *)(((uchar *)dst)+drow);
tloop_start:
      flat16_flat16_trans_il(src, dst, w);
   } while (--h);
}

void flat16_flat16_opaque_clut_ubitmap(grs_bitmap *bm, int x, int y)
{
   int w, h, srow, drow;
   ushort *src, *dst, *clut;

   w = bm->w;
   h = bm->h;
   if ((w<=0)||(h<=0)) {
      Warning(("flat16_flat16_opaque_clut_ubitmap() bitmap has w<=0 or h<=0!\n"));
      return;
   }
   srow = bm->row;
   src = (ushort *)bm->bits;
   drow = grd_bm.row;
   dst = (ushort *)(grd_bm.bits + drow*y + 2*x);
   clut = (ushort *)gr_get_fill_parm();

   goto ocloop_start;

   do {
      src = (ushort *)(((uchar *)src)+srow);
      dst = (ushort *)(((uchar *)dst)+drow);
ocloop_start:
      flat16_flat16_opaque_clut_il(dst, src, w, clut);
   } while (--h);
}

