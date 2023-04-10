/* 
 * $Source: s:/prj/tech/libsrc/g2/RCS/rastel.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:35:19 $
 */

#include <dev2d.h>
#include <fixreal.h>
#include <buffer.h>
#include <raster.h>

grs_raster *gr_rasterize_fix_elipse (fix _x0, fix _y0, fix _a, fix _b)
{
   int x;
   int y_min, y_max;
   real d, dx, ddx, dy, ddy;
   real x0, y0, a, b;
   grs_raster *r, *r0, *r_max;

   x0 = fix_to_real(_x0);
   y0 = fix_to_real(_y0);
   a = fix_to_real(_a);
   b = fix_to_real(_b);
   if (a<REAL_UNIT)
      return NULL;
   if (b<REAL_UNIT)
      return NULL;
   y_min = real_int(y0 - b);

   x = real_rint(x0);
   dx = real_make(x,0) - x0;
   dy = real_make(y_min,0) - y0;
   {
      real foo_x, foo_y;
      foo_x = -real_div_16_16_3(dx, a);
      foo_y = -real_div_16_16_3(dy, b);
      d = real_mul_3_3_3(foo_x,foo_x);
      d += real_mul_3_3_3(foo_y, foo_y);
      d -= REAL_UNIT_3;
   }
   dx = real_div_16_16_3(dx+(REAL_UNIT/2), a);
   dx = 2 * real_div_3_16_3(dx, a);

   ddx = real_div_3_16_3(real_div_16_16_3(2*REAL_UNIT, a), a);

   dy = real_div_16_16_3(dy+(REAL_UNIT/2), b);
   dy = 2 * real_div_3_16_3(dy, b);

   ddy = real_div_3_16_3(real_div_16_16_3(2*REAL_UNIT, b), b);

   while (d >= 0) {
      d += dy;
      dy += ddy;
      y_min++;
   }
   y_max = y_min;

   {
      real _d = d;
      real _dy = dy;
      while (_d < 0) {
         _d += _dy;
         _dy += ddy;
         y_max++;
      }
   }

   if (y_min>=y_max)
      return NULL;
   r0 = gr_alloc_temp((y_max-y_min+1)*sizeof(*r0));
   if (r0==NULL)
      return NULL;
   r0->top = y_min;
   r0->bot = y_max;

   r = r0 + 1;
   r_max = r0 + r0->bot - r0->top;
   while (dy < ddy) {
      while (d < 0) {
         d += dx;
         dx += ddx;
         x++;
      }
      r->right = x - 1;
      d += dy;
      dy += ddy;
      r++;
   }

   if (r<=r_max) {
      do {
         while (d >= 0) {
            dx -= ddx;
            d -= dx;
            x--;
         }
         r->right = x;
         if (r==r_max)
            break;
         d += dy;
         dy += ddy;
         r++;
      } while (TRUE);
   } else {
      do {
         dy -= ddy;
         d -= dy;
         r--;
         while (d >= 0) {
            dx -= ddx;
            d -= dx;
            x--;
         }
      } while (r>r_max);
   }

   while (dy > 0) {
      while (d < 0) {
         dx -= ddx;
         d -=dx;
         x--;
      }
      r->left = x + 1;
      dy -= ddy;
      d -= dy;
      r--;
   }

   while (r > r0) {
      while (d >= 0) {
         d +=dx;
         dx += ddx;
         x++;
      }
      r->left = x;
      dy -= ddy;
      d -= dy;
      r--;
   }
   return r0;
}

