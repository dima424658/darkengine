/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/genline.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:30:11 $
 */

#include <dev2d.h>
#include <mathmac.h>
#include <plytyp.h>

void gen_uline(grs_vertex *v0, grs_vertex *v1) {
   fix x0, y0, x1, y1;          
   int dx, dy, dys;		      /* deltas in x and y */
   int n;
   fix_ulong f, df;
   gdulin_func *seg_func;

   int x, y, xf;
   int *xp, *yp;			   /* pointers to coords */

   if (v1->x < v0->x) {
      grs_vertex *tmp=v0;
      v0 = v1;
      v1 = tmp;
   }
   x0 = v0->x + FIX_HALF; y0 = v0->y + FIX_HALF;
   x1 = v1->x + FIX_HALF; y1 = v1->y + FIX_HALF;
   if ((fix_int(y0)) == (fix_int(y1))) {
      gd_uhline(fix_int(x0), fix_int(y0), fix_int(x1));
      return;
   }

   dx = fix_int (x1) - fix_int (x0);	/* x extent in pixels */
   dys = fix_int (y1) - fix_int (y0);	/* y extent in pixels */
   dy = (dys >= 0) ? dys:-dys;
   if (dx == dy) {
      gdupix_func *pix_func = gd_upix_expose(0,0,0);
      int fc = gr_get_fcolor();

      x = fix_int(x0);
      y = fix_int(y0);
      xf = fix_int(x1);
      dy = (dys>=0) ? 1:-1;
      for (;x<=xf; x++,y+=dy)
         pix_func(fc, x, y);
      return;
   }

   if (dx > dy) {
      xp = &x;
      yp = &y;
      df.f = ((y1 - y0)/dx);
      f.f = y0;
      n = dx + 1;
      seg_func = gd_uhline_expose(0,0,0);
      xf = fix_int(x0);
   } else {
      xp = &y;
      yp = &x;
      if (y0 > y1) {
         y0 = y1; y1 = x0; x0 = x1; x1 = y1;
      }
      df.f = ((x1 - x0)/dy);
      f.f = x0;
      n = dy + 1;
      seg_func = gd_uvline_expose(0,0,0);
      xf = fix_int(y0);
   }
   x = fix_int(x0);
   y = fix_int(y0);
   f.ul = fix_frac(f.f)<<16;
   if (df.f >= 0) {
      df.ul = fix_frac(df.f)<<16;
      do {
         f.ul += df.ul;
         if (f.ul < df.ul) {
            seg_func(x, y, xf);
            *xp = ++xf;
            *yp += 1;
            if (--n == 0)
               return;
         } else {
            if (--n == 0) {
               seg_func(x, y, xf);
               return;
            }
            xf++;
         }
      } while (TRUE);
   } else {
      df.ul = fix_frac(df.f)<<16;
      do {
         f.ul += df.ul;
         if (f.ul >= df.ul) {
            seg_func(x, y, xf);
            *xp = ++xf;
            *yp -= 1;
            if (--n == 0)
               return;
         } else {
            if (--n == 0) {
               seg_func(x, y, xf);
               return;
            }
            xf++;
         }
      } while (TRUE);
   }
}

#pragma off(unreferenced)
#include <lftype.h>
g2ul_func *gen_uline_expose (grs_vertex *v0, grs_vertex *v1)
{
   return gen_uline;
}
#pragma on(unreferenced)
