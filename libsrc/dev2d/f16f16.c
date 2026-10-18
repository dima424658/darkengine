// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16f16.c 1.8 1997/02/17 14:48:45 KEVIN Exp $
// Routines for drawing flat16 bitmaps into a flat16 canvas.

#include <dbg.h>
#include <astring.h>

#include <grd.h>
#include <bitmap.h>
#include <context.h>
#include <f16il.h>

/*
 * True-colour UI replacements commonly arrive as ARGB 4444 bitmaps.  The
 * original flat16 blitters predate that format and copied those words directly
 * into the RGB 565 screen canvas, which both discarded alpha and interpreted
 * the colour nibbles in the wrong positions.  Composite them while they still
 * carry their format tag.  A format-less flat16 canvas is the normal 565 Dark
 * screen; retain the 555 case for legacy display modes.
 */
static void flat16_argb4444_ubitmap(grs_bitmap *bm, int x, int y)
{
   int w, h, srow, drow, dest_555;
   ushort *src, *dst;

   w = bm->w;
   h = bm->h;
   srow = bm->row;
   src = (ushort *)bm->bits;
   drow = grd_bm.row;
   dst = (ushort *)(grd_bm.bits + drow*y + 2*x);
   dest_555 = gr_test_bitmap_format(&grd_bm, BMF_RGB_555);

   while (h-- > 0) {
      int i;
      for (i = 0; i < w; ++i) {
         ushort pixel = src[i];
         int alpha = (pixel >> 12) & 0xf;
         int sr, sg, sb, dr, dg, db;

         if (alpha == 0)
            continue;

         sr = ((pixel >> 8) & 0xf) * 31 / 15;
         sb = (pixel & 0xf) * 31 / 15;
         if (dest_555) {
            sg = ((pixel >> 4) & 0xf) * 31 / 15;
            dr = (dst[i] >> 10) & 0x1f;
            dg = (dst[i] >> 5) & 0x1f;
            db = dst[i] & 0x1f;
            dr = (sr * alpha + dr * (15 - alpha) + 7) / 15;
            dg = (sg * alpha + dg * (15 - alpha) + 7) / 15;
            db = (sb * alpha + db * (15 - alpha) + 7) / 15;
            dst[i] = (ushort)((dr << 10) | (dg << 5) | db);
         } else {
            sg = ((pixel >> 4) & 0xf) * 63 / 15;
            dr = (dst[i] >> 11) & 0x1f;
            dg = (dst[i] >> 5) & 0x3f;
            db = dst[i] & 0x1f;
            dr = (sr * alpha + dr * (15 - alpha) + 7) / 15;
            dg = (sg * alpha + dg * (15 - alpha) + 7) / 15;
            db = (sb * alpha + db * (15 - alpha) + 7) / 15;
            dst[i] = (ushort)((dr << 11) | (dg << 5) | db);
         }
      }
      src = (ushort *)(((uchar *)src) + srow);
      dst = (ushort *)(((uchar *)dst) + drow);
   }
}

static void flat16_argb1555_ubitmap(grs_bitmap *bm, int x, int y)
{
   int w, h, srow, drow, dest_555;
   ushort *src, *dst;

   w = bm->w;
   h = bm->h;
   srow = bm->row;
   src = (ushort *)bm->bits;
   drow = grd_bm.row;
   dst = (ushort *)(grd_bm.bits + drow*y + 2*x);
   dest_555 = gr_test_bitmap_format(&grd_bm, BMF_RGB_555);

   while (h-- > 0) {
      int i;
      for (i = 0; i < w; ++i) {
         ushort pixel = src[i];
         int red, green, blue;
         if (!(pixel & 0x8000))
            continue;
         red = (pixel >> 10) & 0x1f;
         green = (pixel >> 5) & 0x1f;
         blue = pixel & 0x1f;
         if (dest_555)
            dst[i] = (ushort)((red << 10) | (green << 5) | blue);
         else
            dst[i] = (ushort)((red << 11) |
                              ((green * 63 / 31) << 5) | blue);
      }
      src = (ushort *)(((uchar *)src) + srow);
      dst = (ushort *)(((uchar *)dst) + drow);
   }
}

void flat16_flat16_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   void (*memcpy_func)(void *dst, void *src, int count);
   int w, h, srow, drow;
   ushort *src, *dst;

   if (gr_test_bitmap_format(bm, BMF_RGB_1555)) {
      flat16_argb1555_ubitmap(bm, x, y);
      return;
   }
   if (gr_test_bitmap_format(bm, BMF_RGB_4444)) {
      flat16_argb4444_ubitmap(bm, x, y);
      return;
   }

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
          (((uintptr_t)src) & 3) != (((uintptr_t)dst) & 3)) {
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

   if (gr_test_bitmap_format(bm, BMF_RGB_1555)) {
      flat16_argb1555_ubitmap(bm, x, y);
      return;
   }
   if (gr_test_bitmap_format(bm, BMF_RGB_4444)) {
      flat16_argb4444_ubitmap(bm, x, y);
      return;
   }

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

