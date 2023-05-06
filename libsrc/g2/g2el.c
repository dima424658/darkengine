/* 
 * $Source: s:/prj/tech/libsrc/g2/RCS/g2el.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:28:51 $
 */

#include <dev2d.h>
#include <g2buffer.h>
#include <rastel.h>

int g2_elipse (fix x0, fix y0, fix a, fix b)
{
   grs_raster *r0, *r, *r_max;
   int x, x_next, x_last;
   int y, y_max;
   gdulin_func *uhline_func;

   r0 = gr_rasterize_fix_elipse(x0, y0, a, b);
   if (r0==NULL)
      return CLIP_ALL;
   uhline_func=gd_uhline_expose(0,0,0);
   y_max = r0->bot;
   y = r0->top;
   r = r0+1;
   r_max = r + y_max - y;
   gr_hline(r->left, y, r->right);
   x_last = r->left;
   r++;
   y++;
   if (r < r_max) {
      x = r->left;
   }
   while (r+1 < r_max) {
      int check = x_last;
      x_next = (r+1)->left;
      if (check < x_next)
         check = x_next;
      if (x < check)
         gd_hline_opt(x, y, check-1, uhline_func);
      else
         gd_point(x, y);
      y++;
      r++;
      x_last = x;
      x = x_next;
   }
   if (r < r_max)
      gd_hline_opt(r->left, y, r->right, uhline_func);
   y = r0->top;
   r = r0+1;
   x_last = r->right;
   r++;
   y++;
   if (r < r_max) {
      x = r->right;
   }
   while (r+1 < r_max) {
      int check = x_last;
      x_next = (r+1)->right;
      if (check > x_next)
         check = x_next;
      if (x > check)
         gd_hline_opt(check+1, y, x, uhline_func);
      else
         gd_point(x, y);
      y++;
      r++;
      x_last = x;
      x = x_next;
   }
   gr_free_temp(r0);
   return CLIP_NONE;
}

