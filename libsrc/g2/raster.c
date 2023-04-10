/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/raster.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:35:30 $
 *
 */

#include <dev2d.h>
#include <raster.h>
#include <buffer.h>
#include <string.h>

// Returns NULL for empty scanlist
grs_raster *gr_rasterize(grs_vertex **vpl, int n, grs_raster *r)
{
   grs_vertex **p_left, **p_right, **p_last;
   fix x_left, x_right;
   fix dx_left, dx_right;
   int y_min, y_max;
   int y, yl, yr;
   grs_raster *retval;

   p_last = vpl+n-1;
   p_left = vpl;
   y_max = y_min = fix_cint((*vpl)->y);
   for (p_right=vpl+1; p_right<=p_last; p_right++) {
      int y = fix_cint((*p_right)->y);
      if (y < y_min) {
         y_min = y;
         p_left = p_right;
      }
      if (y > y_max) {
         y_max = y;
      }
   }
   p_right=p_left;
   y=yr=yl=y_min;
   if (r==NULL) {
      // if empty scan list, return NULL
      if ((y_max - y) > 0)
         r=gr_alloc_temp((y_max-y+1)*sizeof(*r));
      if (r==NULL)
         return NULL;
   }
   retval=r;
   r->left=y_min;
   r->right=y_max;
   r++;
   while (y<y_max) {
      fix y_prev, y_next;
      grs_vertex *prev, *next;
      int y_limit;

      if (yl==y) {
         prev = *p_left;
         y_prev = prev->y;
         if (--p_left < vpl)
            p_left=p_last;
         y_next=(*p_left)->y;
         while (fix_cint(y_next)<=y) {
            if (fix_cint(y_next)==y) {
               prev = *p_left;
               y_prev = y_next;
            }
            if (--p_left < vpl)
               p_left = p_last;
            y_next = (*p_left)->y;
         }
         next=*p_left;
         x_left=prev->x;
         yl = fix_cint(y_next);
         dx_left=fix_div(next->x-x_left,y_next-y_prev);
         x_left+=fix_mul(dx_left,fix_ceil(y_prev)-y_prev);
      }

      if (yr==y) {
         prev = *p_right;
         y_prev = prev->y;
         if (++p_right > p_last)
            p_right=vpl;
         y_next=(*p_right)->y;
         while (fix_cint(y_next)<=y) {
            if (fix_cint(y_next)==y) {
               prev = *p_right;
               y_prev = y_next;
            }
            if (++p_right > p_last)
               p_right = vpl;
            y_next = (*p_right)->y;
         }
         next=*p_right;
         x_right=prev->x;
         yr = fix_cint(y_next);
         dx_right=fix_div(next->x-x_right,y_next-y_prev);
         x_right+=fix_mul(dx_right,fix_ceil(y_prev)-y_prev);
      }
      if (yl<yr)
         y_limit=yl;
      else
         y_limit=yr;
      do {
         r->left=fix_cint(x_left);
         r->right=fix_cint(x_right);
         x_left += dx_left;
         x_right += dx_right;
         r++;
         y++;
      } while (y<y_limit);
   }
   return retval;
}

void raster_poly(uchar color, int n, grs_vertex **vpl)
{
   grs_raster *r,*r0;
   int y,y_max;
   uchar *p;

   r=r0=gr_rasterize(vpl,n,NULL);
   if (r==NULL) return;
   y=r->left;
   p=grd_bm.bits+grd_bm.row*y;
   y_max=r->right;
   r++;
   while (y<y_max) {
      memset(p + r->left, color, r->right - r->left);
      r++;
      y++;
      p+=grd_bm.row;
   }
   gr_free_temp(r0);
}
