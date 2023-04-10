/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/fl8lin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 09:54:31 $
*/

/* this was originally the guts of flat8_fix_uline */


   fix x0, y0, x1, y1;          
   int dx, dy;		      /* deltas in x and y */
   int n;
   fix_ulong f, df;
   int dp, dpc;

   uchar *p;			   /* pointer into canvas */

   x0 = v0->x + FIX_HALF; y0 = v0->y + FIX_HALF;
   x1 = v1->x + FIX_HALF; y1 = v1->y + FIX_HALF;

   if ((fix_int(y0)) == (fix_int(y1))) {
      if (x0 > x1) {
         y1 = x0; x0 = x1; x1 = y1;
      }
      gr_uhline(fix_int(x0), fix_int(y0), fix_int(x1));
      return;
   }

   dx = fix_int (x1) - fix_int (x0);	/* x extent in pixels */
   dy = fix_int (y1) - fix_int (y0);	/* y extent in pixels */
   if (dx < 0) dx = -dx;
   if (dy < 0) dy = -dy;

   if (dx > dy) {
      if (x0 > x1) {
         x0 = x1; x1 = y0; y0 = y1; y1 = x1;
      }
      df.f = ((y1 - y0)/dx);
      f.f = y0;
      n = dx + 1;
      dp = 1 + fix_int(df.f) * grd_bm.row;
      dpc = dp + grd_bm.row;
   } else {
      if (y0 > y1) {
         y0 = y1; y1 = x0; x0 = x1; x1 = y1;
      }
      df.f = ((x1 - x0)/dy);
      f.f = x0;
      n = dy + 1;
      dp = grd_bm.row + fix_int(df.f);
      dpc = dp + 1;
   }
   p = grd_bm.bits + grd_bm.row * fix_int(y0) + fix_int(x0);
   df.ul = fix_frac(df.f)<<16;
   f.ul = fix_frac(f.f)<<16;

   do {
      flat8_pixel_fill(p, c);
      f.ul += df.ul;
      if (f.ul < df.ul)
         p += dpc;
      else
         p += dp;
   } while (--n);
