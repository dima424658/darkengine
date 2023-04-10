/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16mono.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:43:51 $
 * 
 * Routines for drawing monochrome bitmaps into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <bitmap.h>
#include <grd.h>

void flat16_mono_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   int w, h;         /* working width and height */
   int bitmask;      /* bit from 0-7 in source byte */
   int bm0;
   ushort fc;        /* foreground color */
   uchar *p_src;     /* pointer to source byte */
   ushort *p_dst;
   int p_src_delta, p_dst_delta;

   fc = grd_gc.fcolor;
   h = bm->h;
   p_src = bm->bits;
   p_src_delta = bm->row - ((bm->align + bm->w)>>3);
   p_dst = (ushort *)(grd_bm.bits + y*grd_bm.row + 2*x);
   p_dst_delta = grd_bm.row - 2*bm->w;
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
         bitmask = (carry<<7) + (bitmask>>1);
      }
      p_dst = (ushort *)(((uchar *)p_dst) + p_dst_delta);
      p_src += p_src_delta;
   }
}

void flat16_mono_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   int w, h;         /* working width and height */
   int bitmask;      /* bit from 0-7 in source byte */
   int bm0;
   ushort fc,bc;       /* foreground color */
   uchar *p_src;     /* pointer to source byte */
   ushort *p_dst;
   int p_src_delta, p_dst_delta;

   fc = grd_gc.fcolor;
   bc = grd_gc.bcolor;
   h = bm->h;
   p_src = bm->bits;
   p_src_delta = bm->row - ((bm->align + bm->w)>>3);
   p_dst = (ushort *)(grd_bm.bits + y*grd_bm.row + 2*x);
   p_dst_delta = grd_bm.row - 2*bm->w;
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
      p_dst = (ushort *)(((uchar *)p_dst) + p_dst_delta);
      p_src += p_src_delta;
   }
}

void flat16_mono_opaque_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   int fc_save = grd_gc.fcolor;
   int bc_save = grd_gc.bcolor;

   grd_gc.fcolor = ((uchar *)grd_gc.fill_parm)[fc_save];
   grd_gc.bcolor = ((uchar *)grd_gc.fill_parm)[bc_save];

   flat16_mono_opaque_ubitmap(bm, x, y);

   grd_gc.fcolor = fc_save;
   grd_gc.bcolor = bc_save;
}

void flat16_mono_trans_clut_ubitmap (grs_bitmap *bm, int x, int y)
{
   int fc_save = grd_gc.fcolor;

   grd_gc.fcolor = ((uchar *)grd_gc.fill_parm)[fc_save];

   flat16_mono_opaque_ubitmap(bm, x, y);

   grd_gc.fcolor = fc_save;
}

void flat16_mono_trans_solid_ubitmap (grs_bitmap *bm, int x, int y)
{
   int fc_save = grd_gc.fcolor;

   grd_gc.fcolor = grd_gc.fill_parm;

   flat16_mono_opaque_ubitmap(bm, x, y);

   grd_gc.fcolor = fc_save;
}
