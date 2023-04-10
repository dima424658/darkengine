/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/genf16.c $
 * $Revision: 1.3 $
 * $Author: KEVIN $
 * $Date: 1996/07/03 13:57:15 $
 *
 */

#include <clip.h>
#include <grd.h>
#include <bmftype.h>

/* clip flat8 bitmap against cliprect and jump to unclipped drawer. */
void gen_flat16_bitmap (grs_bitmap *bm, int x, int y, int code, gdubm_func *ubm_func)
{
   int extra;
   grs_bitmap bm_save = *bm;

   /* clip that sucker. */
   if (code & CLIP_LEFT) {             /* off left edge */
      extra = grd_clip.left - x;
      bm->w -= extra;
      bm->bits += 2*extra;
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
