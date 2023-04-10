/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8fl8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:48:12 $
 * 
 * Routines for drawing flat8 bitmaps into a flat8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <string.h>
#include <bank.h>

void bank8_flat8_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   long p;
   int w = bm->w;
   int h = bm->h;
   int srow = bm->row;
   int drow = grd_bm.row;
   uchar *base=gd_bank_p(0);
   int save_bank = gdd_save_bank;

   src = bm->bits;
   p = ((long )grd_bm.bits) + drow*y + x;

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
         memcpy (base+p, src, w);
      } else {
         memcpy (base+p, src, delta);
         gd_inc_bank();
         p -= 0x10000;
         memcpy (base, src+delta, w-delta);
      }
      src += srow;
      p += drow;
   }
   /* synchronize gdd_bank */
   gd_restore_bank(save_bank);
}

void bank8_flat8_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   uchar *src;
   long p;
   int w = bm->w;
   int h = bm->h;
   int srow = bm->row;
   int drow = grd_bm.row;
   uchar *base=gd_bank_p(0);
   int save_bank = gdd_save_bank;

   src = bm->bits;
   p = ((long )grd_bm.bits) + drow*y + x;

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
            uchar c=src[i];
            if (c) base[p+i]=c;
         }
      } else {
         for (i=0; i<delta; i++) {
            uchar c=src[i];
            if (c) base[p+i]=c;
         }
         gd_inc_bank();
         p -= 0x10000;
         for (i=delta; i<w; i++) {
            uchar c=src[i];
            if (c) base[i-delta]=c;
         }
      }
      src += srow;
      p += drow;
   }
   /* synchronize gdd_bank */
   gd_restore_bank(save_bank);
}
