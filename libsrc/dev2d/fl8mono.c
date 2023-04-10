/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/fl8mono.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1997/10/02 10:39:14 $
 * 
 * Routines for drawing monochrome bitmaps into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <bitmap.h>
#include <grd.h>

void flat8_mono_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   int w, h;         /* working width and height */
   int bitmask;      /* bit from 0-7 in source byte */
   int bm0;
   char fc;          /* foreground color */
   uchar *p_src;     /* pointer to source byte */
   uchar *p_dst;
   int p_src_delta, p_dst_delta;

   fc = grd_gc.fcolor;
   h = bm->h;
   p_src = bm->bits;
   p_dst = grd_bm.bits + y*grd_bm.row + x;
   p_src_delta = bm->row - ((bm->align + bm->w)>>3);
   p_dst_delta = grd_bm.row - bm->w;
   bm0 = (1 << (7-(bm->align&7)));

   while (h-- > 0)
   {
      /* set up scanline. */
      bitmask = bm0;
      w = bm->w;

      while (w-- > 0)
      {
         int carry = bitmask&1;
         if (*p_src & bitmask)
            *p_dst = fc;
         p_dst++;
         p_src += carry;
         bitmask = (carry<<7)+(bitmask>>1);
      }
      p_dst += p_dst_delta;
      p_src += p_src_delta;
   }
}

void flat8_mono_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   int w, h;         /* working width and height */
   int bitmask;      /* bit from 0-7 in source byte */
   int bm0;
   char fc,bc;       /* foreground color */
   uchar *p_src;     /* pointer to source byte */
   uchar *p_dst;
   int p_src_delta, p_dst_delta;

   fc = grd_gc.fcolor;
   bc = grd_gc.bcolor;
   h = bm->h;
   p_src = bm->bits;
   p_dst = grd_bm.bits + y*grd_bm.row + x;
   p_src_delta = bm->row - ((bm->align + bm->w)>>3);
   p_dst_delta = grd_bm.row - bm->w;
   bm0 = (1 << (7-(bm->align&7)));

   while (h-- > 0)
   {
      /* set up scanline. */
      bitmask = bm0;
      w = bm->w;

      while (w-- > 0)
      {
         int carry = bitmask&1;
         if (*p_src & bitmask)
            *p_dst = fc;
         else
            *p_dst = bc;
         p_dst++;
         p_src += carry;
         bitmask = (carry<<7) + (bitmask>>1);
      }
      p_dst += p_dst_delta;
      p_src += p_src_delta;
   }
}

void flat8_mono_opaque_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   int fc_save = grd_gc.fcolor;
   int bc_save = grd_gc.bcolor;

   grd_gc.fcolor = ((uchar *)grd_gc.fill_parm)[fc_save];
   grd_gc.bcolor = ((uchar *)grd_gc.fill_parm)[bc_save];

   flat8_mono_opaque_ubitmap(bm, x, y);

   grd_gc.fcolor = fc_save;
   grd_gc.bcolor = bc_save;
}

void flat8_mono_trans_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   int fc_save = grd_gc.fcolor;

   grd_gc.fcolor = ((uchar *)grd_gc.fill_parm)[fc_save];

   flat8_mono_trans_ubitmap(bm, x, y);

   grd_gc.fcolor = fc_save;
}

void flat8_mono_trans_solid_ubitmap (grs_bitmap *bm, int x, int y)
{
   int fc_save = grd_gc.fcolor;

   grd_gc.fcolor = grd_gc.fill_parm;

   flat8_mono_trans_ubitmap(bm, x, y);

   grd_gc.fcolor = fc_save;
}
