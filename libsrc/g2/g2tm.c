/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/g2tm.c $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1997/01/24 15:29:41 $
 *
 * 2D polygon clipping routines.
 *
 */

#include <g2buffer.h>
#include <grtm.h>
#include <g2clip.h>

typedef void (g2ut_func)(grs_bitmap* bm, int n, grs_vertex** vpl);

static int do_map(grs_bitmap *bm, int n, grs_vertex **vpl, int clip_type, g2ut_func *f)
{
   grs_vertex **cvpl=NULL;
   int m, code;

   m=g2_clip_polygon(n, clip_type, vpl, &cvpl);
   if (m<3) {
      code = CLIP_ALL;
   } else {
      f(bm, m, cvpl);
      code = CLIP_NONE;
   }
   gr_free_temp(cvpl);
   return code;
}

g2ut_func *lmap_func=NULL;

static void per_map_cb(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   int i;
   for (i=0; i<n; i++) {
      vpl[i]->u = fix_div(vpl[i]->u, vpl[i]->w);
      vpl[i]->v = fix_div(vpl[i]->v, vpl[i]->w);
   }
   lmap_func(bm, n, vpl);
   for (i=0; i<n; i++) {
      vpl[i]->u = fix_mul(vpl[i]->u, vpl[i]->w);
      vpl[i]->v = fix_mul(vpl[i]->v, vpl[i]->w);
   }
}

#define gr_set_fix_cliprect_unsafe(l, t, r, b) \
   grd_gc.clip.f.left = (l), \
   grd_gc.clip.f.top = (t), \
   grd_gc.clip.f.right = (r), \
   grd_gc.clip.f.bot = (b)

#define G2C_PM_DELTA 0x00100000 // 16.0
#define G2C_PM_MASK  0xFFF00000 // mask out low bits

static void per_map_shell(grs_bitmap *bm, int n, grs_vertex **vpl, int clip_type)
{
   int i;
   fix x_min, x_max, y_min, y_max;
   fix x, y, next_x, next_y;
   fix left, right, top, bot;
   grs_vertex *vp = *vpl;

   gr_get_fix_cliprect(&left, &top, &right, &bot);

   x_min = x_max = vp->x;
   y_min = y_max = vp->y;

   for (i=1; i<n; i++) {
      vp = vpl[i];
      if (vp->x < x_min)
         x_min = vp->x;
      if (vp->x > x_max)
         x_max = vp->x;
      if (vp->y < y_min)
         y_min = vp->y;
      if (vp->y > y_max)
         y_max = vp->y;
   }
   x_min = (x_min&G2C_PM_MASK);
   x_max = (x_max&G2C_PM_MASK)+G2C_PM_DELTA;
   y_min = (y_min&G2C_PM_MASK);
   y_max = (y_max&G2C_PM_MASK)+G2C_PM_DELTA;

   if ((x_max-x_min == G2C_PM_DELTA) &&
       (y_max-y_min == G2C_PM_DELTA)) {
      lmap_func(bm, n, vpl);
      return;
   }

   for (i=0; i<n; i++) {
      vpl[i]->u = fix_mul(vpl[i]->u, vpl[i]->w);
      vpl[i]->v = fix_mul(vpl[i]->v, vpl[i]->w);
   }
   for (x = x_min; x < x_max; x = next_x) {
      next_x = x + G2C_PM_DELTA;
      if (next_x > x_max)
         next_x = x_max;
      for (y = y_min; y < y_max; y = next_y) {
         next_y = y + G2C_PM_DELTA;
         if (next_y > y_max)
            next_y = y_max;
         gr_set_fix_cliprect_unsafe(x, y, next_x, next_y);
         do_map(bm, n, vpl, clip_type, per_map_cb);
      }
   }
   gr_set_fix_cliprect(left, top, right, bot);
   return;
}

void gen_upmap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   lmap_func = gr_lin_umap_expose(bm, n, vpl);
   per_map_shell(bm, n, vpl, G2C_CLIP_UVW);
}

#pragma off(unreferenced)
g2ut_func *gen_upmap_expose(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   return gen_upmap;
}
#pragma on(unreferenced)

void gen_lit_upmap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   lmap_func = gr_lit_lin_umap_expose(bm, n, vpl);
   per_map_shell(bm, n, vpl, G2C_CLIP_UVWI);
}

#pragma off(unreferenced)
g2ut_func *gen_lit_upmap_expose(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   return gen_lit_upmap;
}
#pragma on(unreferenced)

int gr_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   return do_map(bm, n, vpl, G2C_CLIP_UV,
      gr_lin_umap_expose(bm, n, vpl));
}

int gr_lit_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   return do_map(bm, n, vpl, G2C_CLIP_UVI,
      gr_lit_lin_umap_expose(bm, n, vpl));
}

// actually, these won't work 'cuz need u,v --> u,v *w, etc

int gr_per_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   return do_map(bm, n, vpl, G2C_CLIP_UVW,
      gr_per_umap_expose(bm, n, vpl));
}

int gr_lit_per_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   return do_map(bm, n, vpl, G2C_CLIP_UVWI,
      gr_lit_per_umap_expose(bm, n, vpl));
}
