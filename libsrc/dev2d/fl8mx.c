/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8mx.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:34:58 $
 *
 * Routines for drawing flat8 bitmaps onto a mode X canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <mxlatch.h>

void flat8_modex_trans_ubitmap (grs_bitmap *bm, int x, int y)
{
   int i,j,k;
   uchar *src, *dst;
   uchar *s, *d;
   uchar *p;
   short w, h;
   uchar save_wlatch;

   wlatch_start(save_wlatch);

   w = bm->w;
   h = bm->h;
   if ((h<=0)||(w<=0)) return;
   p = grd_bm.bits+grd_bm.row*y;
   for (i=0; i<4; i++) {
      int z = i + bm->align;
      src=bm->bits+(z>>2);
      dst=p+x;
      modex_force_wlatch(1<<(z&3));
      for (j=(w+3)>>2; j>0; j--) {
         s=src; d=dst;
         for (k=0; k<h; k++) {
            if (*s) *d=*s;
            s+=bm->row;
            d+=grd_bm.row;
         }
         src++;
         dst+=4;
      }
      x++;
      if (--w==0) break;
   }

   wlatch_restore(save_wlatch);
}

void flat8_modex_opaque_ubitmap (grs_bitmap *bm, int x, int y)
{
   int i,j,k;
   uchar *src, *dst;
   uchar *s, *d;
   uchar *p;
   short w, h;
   uchar save_wlatch;

   wlatch_start(save_wlatch);

   w = bm->w;
   h = bm->h;
   if ((h<=0)||(w<=0)) return;
   p = grd_bm.bits+grd_bm.row*y;
   for (i=0; i<4; i++) {
      int z = i + bm->align;
      src=bm->bits+(z>>2);
      dst=p+x;
      modex_force_wlatch(1<<(z&3));
      for (j=(w+3)>>2; j>0; j--) {
         s=src; d=dst;
         for (k=0; k<h; k++) {
            *d=*s;
            s+=bm->row;
            d+=grd_bm.row;
         }
         src++;
         dst+=4;
      }
      x++;
      if (--w==0) break;
   }

   wlatch_restore(save_wlatch);
}
