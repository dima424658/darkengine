/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/lmap.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:35:10 $
 *
 * Linear mapper shell.
 *
 * This file is part of the g2 library.
 *
 */

#include <tmapd.h>
#include <tmapfcn.h>
#include <raster.h>
#include <buffer.h>
#include <mprintf.h>

void lmap_shell(grs_vertex **vpl)
{
   grs_vertex *vp=vpl[0];
   grs_raster *r0, *r;
   g2s_tmap_info *ti;
   fix u0, v0;
   int x0, y0, y_max;

   g2_calc_deltas(vpl, TMF_CALC_UV);
   r0 = gr_rasterize(vpl, 3, NULL);
   if (r0==NULL)
      return;
   ti = &g2d_tmap_info;
   x0 = fix_int(vp->x);
   y0 = fix_int(vp->y);
   u0 = vp->u - fix_mul(fix_frac(vp->x), ti->dux)
              - fix_mul(fix_frac(vp->y), ti->duy);
   v0 = vp->v - fix_mul(fix_frac(vp->x), ti->dvx)
              - fix_mul(fix_frac(vp->y), ti->dvy);
   ti->y = r0->left;
   y_max = r0->right;
   ti->p_dest = grd_bm.bits + ti->y*grd_bm.row;
   r = r0+1;
   u0 += ti->duy*(ti->y-y0);
   v0 += ti->dvy*(ti->y-y0);

   while (ti->y<y_max) {
      int dx = r->left-x0;
      fix u = u0 + dx*ti->dux;
      fix v = v0 + dx*ti->dvx;
      ti->il_func(r->left, r->right, u, v);
      u0 += ti->duy;
      v0 += ti->dvy;
      ti->y++;
      r++;
   }
   gr_free_temp(r0);
}

void lit_lmap_shell(grs_vertex **vpl)
{
   grs_vertex *vp=vpl[0];
   grs_raster *r0, *r;
   g2s_tmap_info *ti;
   fix u0, v0, i0;
   int x0, y0, y_max;

   g2_calc_deltas(vpl, TMF_CALC_UVI);
   r0 = gr_rasterize(vpl, 3, NULL);
   if (r0==NULL)
      return;
   ti = &g2d_tmap_info;
   x0 = fix_int(vp->x);
   y0 = fix_int(vp->y);
   u0 = vp->u - fix_mul(fix_frac(vp->x), ti->dux)
              - fix_mul(fix_frac(vp->y), ti->duy);
   v0 = vp->v - fix_mul(fix_frac(vp->x), ti->dvx)
              - fix_mul(fix_frac(vp->y), ti->dvy);
   i0 = vp->i - fix_mul(fix_frac(vp->x), ti->dix)
              - fix_mul(fix_frac(vp->y), ti->diy);
   ti->y = r0->left;
   y_max = r0->right;
   ti->p_dest = grd_bm.bits + ti->y*grd_bm.row;
   r = r0+1;
   u0 += ti->duy*(ti->y-y0);
   v0 += ti->dvy*(ti->y-y0);
   i0 += ti->diy*(ti->y-y0);

   while (ti->y<y_max) {
      int dx = r->left-x0;
      fix u = u0 + dx*ti->dux;
      fix v = v0 + dx*ti->dvx;
      ti->i = i0 + dx*ti->dix;
      ti->il_func(r->left, r->right, u, v);
      u0 += ti->duy;
      v0 += ti->dvy;
      i0 += ti->diy;
      ti->y++;
      r++;
   }
   gr_free_temp(r0);
}
