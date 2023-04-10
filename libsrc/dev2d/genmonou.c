/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/genmonou.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:39:02 $
 * 
 * Routines for drawing monochrome bitmaps into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <gdpix.h>

void gen_mono_trans_ubitmap (grs_bitmap *bm, int x0, int y0)
{
   int x, y;         /* working pixel position */
   int xf, yf;
   int bitmask;      /* bit from 0-7 in source byte */
   int bm0;
   char fc;          /* foreground color */
   uchar *p_src;     /* pointer to source byte */
   int p_src_delta;

   fc = grd_gc.fcolor;
   p_src = bm->bits;
   p_src_delta = bm->row - ((bm->align + bm->w)>>3);
   bm0 = (1 << (7-(bm->align&7)));
   xf = x0+bm->w, yf = y0+bm->h;

   for (y=y0; y<yf; y++)
   {
      /* set up scanline. */
      bitmask = bm0;

      for (x=x0; x<xf; x++)
      {
         int carry = bitmask&1;
         if (*p_src & bitmask)
            gd_upix8(fc, x, y);
         p_src += carry;
         bitmask = (carry<<7) + (bitmask>>1);
      }
      p_src += p_src_delta;
   }
}

void gen_mono_opaque_ubitmap (grs_bitmap *bm, int x0, int y0)
{
   int x, y;         /* working pixel position */
   int xf, yf;
   int bitmask;      /* bit from 0-7 in source byte */
   int bm0;
   char fc,bc;          /* foreground color */
   uchar *p_src;     /* pointer to source byte */
   int p_src_delta;

   fc = grd_gc.fcolor, bc=grd_gc.bcolor;
   p_src = bm->bits;
   p_src_delta = bm->row - ((bm->align + bm->w)>>3);
   bm0 = (1 << (7-(bm->align&7)));
   xf = x0+bm->w, yf = y0+bm->h;

   for (y=y0; y<yf; y++)
   {
      /* set up scanline. */
      bitmask = bm0;

      for (x=x0; x<xf; x++)
      {
         int carry = bitmask&1;
         if (*p_src & bitmask)
            gd_upix8(fc, x, y);
         else
            gd_upix8(bc, x, y);
         p_src += carry;
         bitmask = (carry<<7) + (bitmask>>1);
      }
      p_src += p_src_delta;
   }
}



