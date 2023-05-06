/* 
 * $Source: s:/prj/tech/libsrc/g2/RCS/g2ov.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:28:58 $
 */

#include <dev2d.h>
#include <g2buffer.h>
#include <rastel.h>

int g2_oval (fix x0, fix y0, fix a, fix b)
{
   grs_raster *r0, *r;
   int y, y_max;
   gdulin_func *uhline_func;

   r0 = gr_rasterize_fix_elipse(x0, y0, a, b);
   if (r0==NULL) {
      return CLIP_ALL;
   }
   uhline_func=gd_uhline_expose(0,0,0);
   y = r0->top;
   y_max = r0->bot;
   r = r0+1;
   while (y < y_max) {
      gd_hline_opt(r->left, y, r->right, uhline_func);
      y++;
      r++;
   }
   gr_free_temp(r0);
   return CLIP_NONE;
}
