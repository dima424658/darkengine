/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/genf16u.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/05/07 13:17:44 $
 * 
 * Routines for drawing flat8 bitmaps into a flat8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <gdpix.h>

void gen_flat16_trans_ubitmap (grs_bitmap *bm, int x0, int y0)
{
   int x, y;
   int xf = x0+bm->w;
   int yf = y0+bm->h;
   int delta = bm->row - 2*bm->w;
   ushort *src = (ushort *)bm->bits;
   gdupix_func *p16_func = gd_upix16_expose(0,0,0);

   for (y=y0; y<yf; y++) {
      for (x=x0; x<xf; x++) {
         int c=*src++;
         if (c!=grd_chroma_key)
            p16_func(c, x, y);
      }
      src = (ushort *)(((uchar *)src) + delta);
   }
}

void gen_flat16_opaque_ubitmap (grs_bitmap *bm, int x0, int y0)
{
   int x, y;
   int xf = x0+bm->w;
   int yf = y0+bm->h;
   int delta = bm->row - 2*bm->w;
   ushort *src = (ushort *)bm->bits;
   gdupix_func *p16_func = gd_upix16_expose(0,0,0);

   for (y=y0; y<yf; y++) {
      for (x=x0; x<xf; x++)
         p16_func(*src++, x, y);
      src = (ushort *)(((uchar *)src) + delta);
   }
}


