/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/genfl8u.c $
 * $Revision: 1.3 $
 * $Author: KEVIN $
 * $Date: 1996/08/19 15:54:46 $
 * 
 * Routines for drawing flat8 bitmaps into a flat8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <gdpix.h>
#include <pal16.h>

void gen_flat8_trans_ubitmap (grs_bitmap *bm, int x0, int y0)
{
   int x, y;
   int xf = x0+bm->w;
   int yf = y0+bm->h;
   int delta = bm->row-bm->w;
   uchar *src = bm->bits;
   gdupix_func *p8_func = gd_upix8_expose(0,0,0);
   pixpal = (void *)grd_pal16_list[bm->align];

   for (y=y0; y<yf; y++) {
      for (x=x0; x<xf; x++) {
         uchar c=*src++;
         if (c!=0)
            p8_func(c, x, y);
      }
      src += delta;
   }
}

void gen_flat8_opaque_ubitmap (grs_bitmap *bm, int x0, int y0)
{
   int x, y;
   int xf = x0+bm->w;
   int yf = y0+bm->h;
   int delta = bm->row-bm->w;
   uchar *src = bm->bits;
   gdupix_func *p8_func = gd_upix8_expose(0,0,0);
   pixpal = (void *)grd_pal16_list[bm->align];

   for (y=y0; y<yf; y++) {
      for (x=x0; x<xf; x++)
         p8_func(*src++, x, y);
      src += delta;
   }
}


