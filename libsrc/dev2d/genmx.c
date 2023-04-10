/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/genmx.c $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1996/07/03 13:58:49 $
 * 
 * Routines for clipping monochrome bitmaps to a rectangle.
 *
 * This file is part of the dev2d library.
 *
 */

#include <clip.h>
#include <grd.h>
#include <bmftype.h>

void gen_modex_bitmap (grs_bitmap *bm, int x, int y, int code, gdubm_func *ubm_func)
{
   int extra;
   grs_bitmap bm_save = *bm;

   if (code & CLIP_LEFT) {             /* off left edge */
      extra = grd_clip.left - x;
      bm->w -= extra;
      extra += bm->align;
      bm->align = extra&3;
      bm->bits += extra>>2;
      x = grd_clip.left;
   }
   if (code & CLIP_RIGHT) {            /* off right edge */
      bm->w = grd_clip.right - x;
   }
   if (bm->w<=0) goto punt;

   if (code & CLIP_TOP) {             /* off top */
      extra = grd_clip.top - y;
      bm->h -= extra;
      bm->bits += bm->row*extra;
      y = grd_clip.top;
   }
   if (code & CLIP_BOT) {      /* off bottom */
      bm->h = grd_clip.bot - y;
   }
   if (bm->h>0)
      ubm_func(bm, x, y);

punt:
   *bm = bm_save;
}
