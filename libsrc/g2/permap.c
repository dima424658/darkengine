// $Header: x:/prj/tech/libsrc/g2/RCS/permap.c 1.3 1997/05/16 09:53:23 KEVIN Exp $
// subdividing perspective mapper.

#include <dev2d.h>
#include <buffer.h>
#include <g2clip.h>
#include <g2tm.h>
#include <tmapd.h>
                       
int g2_map(int n, g2s_point **vpl)
{
   g2s_point **cvpl=NULL;
   int m, code;

   m=g2_clip_poly(n, g2d_pp.clip_type, vpl, &cvpl);
   if (m<3) {
      code = CLIP_ALL;
   } else {
      g2_umap(m, cvpl);
      code = CLIP_NONE;
   }
   if ((cvpl!=NULL)&&(cvpl!=vpl))
      gr_free_temp(cvpl);
   return code;
}

int g2_per_map(int n, g2s_point **vpl)
{
   g2s_point **cvpl=NULL;
   int m, code;

   m=g2_clip_poly(n, g2d_pp.clip_type, vpl, &cvpl);
   if (m<3) {
      code = CLIP_ALL;
   } else {
      g2_per_umap(m, cvpl);
      code = CLIP_NONE;
   }
   if ((cvpl!=NULL)&&(cvpl!=vpl))
      gr_free_temp(cvpl);
   return code;
}

#define gr_set_fix_cliprect_unsafe(l, t, r, b) \
   grd_gc.clip.f.left = (l), \
   grd_gc.clip.f.top = (t), \
   grd_gc.clip.f.right = (r), \
   grd_gc.clip.f.bot = (b)

#define G2C_PM_DELTA 0x00100000 // 16.0
#define G2C_PM_MASK  0xFFF00000 // mask out low bits

int g2_per_umap(int n, g2s_point **vpl)
{
   int i;
   fix x_min, x_max, y_min, y_max;
   fix x, y, next_x, next_y;
   fix left, right, top, bot;
   g2s_point *vp = *vpl;

   for (i=0; i<n; i++)
      vpl[i]->flags = 0;

   gr_get_fix_cliprect(&left, &top, &right, &bot);

   x_min = x_max = vp->sx;
   y_min = y_max = vp->sy;

   for (i=1; i<n; i++) {
      vp = vpl[i];
      if (vp->sx < x_min)
         x_min = vp->sx;
      if (vp->sx > x_max)
         x_max = vp->sx;
      if (vp->sy < y_min)
         y_min = vp->sy;
      if (vp->sy > y_max)
         y_max = vp->sy;
   }
   x_min = (x_min&G2C_PM_MASK);
   x_max = (x_max&G2C_PM_MASK)+G2C_PM_DELTA;
   y_min = (y_min&G2C_PM_MASK);
   y_max = (y_max&G2C_PM_MASK)+G2C_PM_DELTA;

   if ((x_max-x_min == G2C_PM_DELTA) &&
       (y_max-y_min == G2C_PM_DELTA)) {
      g2_umap(n, vpl);
      return CLIP_NONE;
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
         g2_map(n, vpl);
      }
   }

   g2_clip_fixup(n, vpl);
   gr_set_fix_cliprect(left, top, right, bot);
   return CLIP_NONE;
}
