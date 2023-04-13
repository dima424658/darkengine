/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/clpfl8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:00:56 $
 * 
 * Routines for clipping flat 8 bitmaps to a rectangle.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <clip2d.h>

// This is legacy code for terra nova, but won't be linked in
// any apps that don't use it.

/* takes bitmap structure and pointers to x & y location to draw bitmap.
   clips it against the current clipping rectangle, modifying x,y,w,h & bits,
   so these should be saved by caller if needed. returns clip code. */
int gr_clip_flat8_bitmap (grs_bitmap *bm, short *x, short *y)
{
   int code = CLIP_NONE;
   int extra;
   int l, r, t, b;

   l=*x; r=l+bm->w; t=*y; b=t+bm->h;
   if (l>grd_clip.right || r<grd_clip.left ||
       t>grd_clip.bot || b<grd_clip.top)
      /* bitmap is completely clipped. */
      return CLIP_ALL;

   if (l < grd_clip.left) {
      /* bitmap is off the left edge of the window. */
      extra = grd_clip.left-l;
      bm->w -= extra;
      bm->bits += extra;
      *x = grd_clip.left;
      code |= CLIP_LEFT;
   }
   if (r > grd_clip.right) {
      /* off the right edge of the window. */
      bm->w -= r-grd_clip.right;
      code |= CLIP_RIGHT;
   }
   if (t < grd_clip.top) {
      /* off the top of the window. */
      extra = grd_clip.top-t;
      bm->h -= extra;
      bm->bits += bm->row*extra;
      *y = grd_clip.top;
      code |= CLIP_TOP;
   }
   if (b > grd_clip.bot) {
      /* off the bottom of the window. */
      bm->h -= b-grd_clip.bot;
      code |= CLIP_BOT;
   }

   return code;
}
