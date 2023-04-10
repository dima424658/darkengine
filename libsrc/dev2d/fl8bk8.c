/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8bk8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:31:26 $
 * 
 * Routines for drawing flat8 bitmaps into a flat8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <string.h>
#include <bank.h>

void flat8_bank8_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *dst;
   long p;
   int w = bm->w;
   int h = bm->h;
   int srow = bm->row;
   int drow = grd_bm.row;
   uchar *base=gd_bank_p(0);
   int save_bank = gdd_save_bank;

   p = ((long )bm->bits);
   dst = grd_bm.bits + drow*y + x;

   gd_set_bank(p>>16);

   p &= 0xffff;
   while (h--) {
      int delta;

      if (p>=0x10000) {
         gd_inc_bank();
         p -= 0x10000;
      }
      delta = 0x10000-p;
      if (delta >= w) {
         memcpy (dst, base+p, w);
      } else {
         memcpy (dst, base+p, delta);
         gd_inc_bank();
         p -= 0x10000;
         memcpy (dst+delta, base, w-delta);
      }
      dst += drow;
      p += srow;
   }
   /* synchronize gdd_bank */
   gd_restore_bank(save_bank);
}

void flat8_bank8_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *dst;
   long p;
   int w = bm->w;
   int h = bm->h;
   int srow = bm->row;
   int drow = grd_bm.row;
   uchar *base=gd_bank_p(0);
   int save_bank = gdd_save_bank;

   p = ((long )bm->bits);
   dst = grd_bm.bits + drow*y + x;

   gd_set_bank(p>>16);

   p &= 0xffff;
   while (h--) {
      int i, delta;

      if (p>=0x10000) {
         gd_inc_bank();
         p -= 0x10000;
      }
      delta = 0x10000-p;
      if (delta >= w) {
         for (i=0; i<w; i++) {
            uchar c=base[p+i];
            if (c) dst[i]=c;
         }
      } else {
         for (i=0; i<delta; i++) {
            uchar c=base[p+i];
            if (c) dst[i]=c;
         }
         gd_inc_bank();
         p -= 0x10000;
         for (i=delta; i<w; i++) {
            uchar c=base[i-delta];
            if (c) dst[i]=c;
         }
      }
      dst += drow;
      p += srow;
   }
   /* synchronize gdd_bank */
   gd_restore_bank(save_bank);
}
