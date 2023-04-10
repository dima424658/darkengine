// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16fl8.c 1.5 1996/09/09 14:15:00 KEVIN Exp $

// Routines for drawing flat8 bitmaps into a flat16 canvas.

#include <string.h>
#include <grd.h>
#include <pal16.h>
#include <dbg.h>

typedef void (blit_inner_loop)(uchar *dst, uchar *src, int count);

static void blit_shell(grs_bitmap *bm, int x, int y, blit_inner_loop *il)
{
   uchar *src, *dst;
   int w = bm->w;
   int h = bm->h;
   void *pp_save = pixpal;

   pixpal = (void *)(grd_pal16_list[bm->align]);
   if (pixpal == NULL) {
      Warning(("flat16_flat8_ubitmap(): Bad palette! index=%i\n", bm->align));
      return;
   }

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + 2*x;

   while (h--) {
      il(dst, src, w);
      src += bm->row;
      dst += grd_bm.row;
   }
   pixpal = pp_save;
}

extern blit_inner_loop flat16_flat8_opaque_inner_loop;

void flat16_flat8_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   blit_shell(bm, x, y, flat16_flat8_opaque_inner_loop);
}

void slow_trans_il(uchar *dst, uchar *src, int n)
{
   ushort *pal=(ushort *)pixpal;
   ushort *d16=(ushort *)dst;
   int i;

   for (i=0; i<n; i++) {
      uchar c=src[i];
      if (c!=0)
         d16[i] = pal[c];
   }
}

extern blit_inner_loop flat16_flat8_trans_inner_loop;

void flat16_flat8_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
//   blit_shell(bm, x, y, flat16_flat8_trans_inner_loop);
   blit_shell(bm, x, y, slow_trans_il);
}

static void flat16_flat8_opaque_clut_inner_loop(uchar *dst, uchar *src, int count)
{
   ushort *clut = (ushort *)(grd_gc.fill_parm);
   ushort *d16 = (ushort *)dst;
   ushort *pal = (ushort *)pixpal;
   int i;

   for (i=0; i<count; i++) {
      int c16 = pal[src[i]];
      d16[i]=clut[c16&255]+clut[256+(c16>>8)];
   }
}

void flat16_flat8_opaque_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   blit_shell(bm, x, y, flat16_flat8_opaque_clut_inner_loop);
}

static void flat16_flat8_trans_clut_inner_loop(uchar *dst, uchar *src, int count)
{
   ushort *clut = (ushort *)(grd_gc.fill_parm);
   ushort *d16 = (ushort *)dst;
   ushort *pal = (ushort *)pixpal;
   int i;

   for (i=0; i<count; i++)
      if (src[i]!=0) {
         int c16 = pal[src[i]];
         d16[i]=clut[c16&255]+clut[256+(c16>>8)];
      }
}

void flat16_flat8_trans_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   blit_shell(bm, x, y, flat16_flat8_trans_clut_inner_loop);
}

static void flat16_flat8_trans_solid_inner_loop(uchar *dst, uchar *src, int count)
{
   ushort c = grd_gc.fill_parm;
   ushort *d16 = (ushort *)dst;
   int i;

   for (i=0; i<count; i++)
      if (src[i]!=0)
         d16[i]=c;
}

void flat16_flat8_trans_solid_ubitmap (grs_bitmap *bm, int x, int y)
{
   blit_shell(bm, x, y, flat16_flat8_trans_solid_inner_loop);
}

