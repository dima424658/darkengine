// $Header: x:/prj/tech/libsrc/g2/RCS/trigrad.c 1.6 1998/04/28 14:29:49 KEVIN Exp $
//
// generic linear deltas calculation function.
// used for gouraud shaded and/or texture mapped triangles.

#include <g2spoint.h>
#include <plyparam.h>
#include <trirast.h>
#include <tmapd.h>

/*  for a true linear mapping, we posit
 *  i=f(x,y)=a*x + b*y +c
 *  Solving the 3 equations for three points from the
 *  vertex pointer list, we have
 *    a=(y10*i20-y20*i10)/(y10*x20-y20*x10); likewise for b
 *  where fij=fi-fj.
 */

#define f2to32 4294967296.0

#define OVERFLOW 0x80000000

void gen_triangle_gradients(g2s_point *v0, g2s_point *v1, g2s_point *v2, g2s_poly_params *tp)
{
   float r, x10, y10, x20, y20;
   int i;
   ulong mask,flags;

   flags = tp->flags&PPF_MASK;
   if (flags>0) {
      x10=v1->sx-v0->sx;
      y10=v1->sy-v0->sy;
      x20=v2->sx-v0->sx;
      y20=v2->sy-v0->sy;

      r = f2to32/(y10*x20 - y20*x10);
   }
// divide overlap!
   tp->ytop = fix_cint(v0->sy);
   tp->yright = fix_cint(v1->sy);
   tp->yleft = fix_cint(v2->sy);

   if (tp->dy == 1)
      tp->y = tp->ytop;
   else
      tp->p = grd_bm.bits + tp->ytop*tp->canvas_row;

   for (mask = 1, i = 0; mask <= flags; mask += mask, i++)
   {
      float c10, c20, scale, dc;

      if (!(mask&flags))
         continue;

      scale = r * tp->scale[i];
      c10=v1->coord[i]-v0->coord[i];
      c20=v2->coord[i]-v0->coord[i];
      dc = (y10*c20-y20*c10)*scale;
      if ((dc > ((float )FIX_MAX))||(dc < ((float)FIX_MIN))) {
         tp->dcx[i] =
         tp->dcy[i] = 0;
      } else {
         tp->dcx[i] = dc;
         dc = (x20*c10-x10*c20)*scale;
         if ((dc > ((float )FIX_MAX))||(dc < ((float)FIX_MIN))) {
            tp->dcx[i] =
            tp->dcy[i] = 0;
         } else {
            tp->dcy[i] = dc;
         }
      }
   }
   if (tp->flags & PPF_MUNGE) {
      grs_bitmap *bm = tp->bm;
      g2d_tmap_info.du_frac = tp->dux<<16;
      g2d_tmap_info.dv_frac = tp->dvx<<16;
      g2d_tmap_info.dsrc[1] = fix_int(tp->dux) + fix_int(tp->dvx)*bm->row;
      g2d_tmap_info.dsrc[0] = g2d_tmap_info.dsrc[1] + bm->row;
   }
}
