// $Header: x:/prj/tech/libsrc/dev2d/RCS/fl8fl8.c 1.6 1997/02/07 11:21:45 KEVIN Exp $
// Routines for drawing flat8 bitmaps into a flat8 canvas.

#include <dbg.h>
#include <astring.h>

#include <grd.h>
#include <bitmap.h>
#include <context.h>

void flat8_flat8_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   uchar *dst;
   int w = bm->w;
   int h = bm->h;
   void (*memcpy_func)(void *dst, void *src, int count);

   Assrt((w>0)&&(h>0));

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + x;

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
   while (h--) {
      memcpy_func (dst, src, w);
      src += bm->row;
      dst += grd_bm.row;
   }
}

void flat8_flat8_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   uchar *dst;
   int w = bm->w;
   int h = bm->h;
   int i;

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + x;

   while (h--) {
      for (i=0; i<w; i++)
         if (src[i]!=0) dst[i]=src[i];
      src += bm->row;
      dst += grd_bm.row;
   }
}

void flat8_flat8_opaque_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   uchar *dst;
   uchar *clut = (uchar *)gr_get_fill_parm();
   int w = bm->w;
   int h = bm->h;

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + x;

   while (h--) {
      int i;
      for (i=0; i<w; i++)
         if (src[i]!=0) dst[i]=clut[src[i]];
      src += bm->row;
      dst += grd_bm.row;
   }
}

void flat8_flat8_trans_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   uchar *dst;
   uchar *clut = (uchar *)gr_get_fill_parm();
   int w = bm->w;
   int h = bm->h;

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + x;

   while (h--) {
      int i;
      for (i=0; i<w; i++)
         if (src[i]!=0) dst[i]=clut[src[i]];
      src += bm->row;
      dst += grd_bm.row;
   }
}

void flat8_flat8_trans_solid_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   uchar *dst;
   int w = bm->w;
   int h = bm->h;
   uchar c = (uchar )gr_get_fill_parm();

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + x;

   while (h--) {
      int i;
      for (i=0; i<w; i++)
         if (src[i]!=0) dst[i]=c;
      src += bm->row;
      dst += grd_bm.row;
   }
}
