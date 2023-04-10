/* 
 * $Source: x:/prj/tech/libsrc/g2/RCS/g2ring.c $
 * $Revision: 1.2 $
 * $Author: buzzard $
 * $Date: 1997/10/31 17:49:41 $
 */

#include <dev2d.h>
#include <buffer.h>
#include <rastel.h>

bool gd_secret_hack_nest_rings;

int g2_ring (fix x0, fix y0, fix co, fix ci)
{
   grs_raster *r0, *r, *r0i, *ri;
   int y, y_max;
   int y_in_min, y_in_max;
   fix a_o, b_o, a_i, b_i;
   gdulin_func *uhline_func;

   if (co < ci) {
      fix foo=co;
      co=ci;
      ci=foo;
   }

   a_o = co*grd_bm.w;
   a_i = ci*grd_bm.w;
   b_o = fix_div(a_o, grd_cap->aspect);
   b_i = fix_div(a_i, grd_cap->aspect);

   r0 = gr_rasterize_fix_elipse(x0, y0, a_o, b_o);
   if (r0==NULL)
      return CLIP_ALL;
   uhline_func=gd_uhline_expose(0,0,0);
   y = r0->top;
   y_max = r0->bot;
   r = r0+1;
   r0i = gr_rasterize_fix_elipse(x0, y0, a_i, b_i);
   if (r0i!=NULL) {
      y_in_min = r0i->top;
      y_in_max = r0i->bot;
      ri = r0i+1;
      while (y < y_in_min) {
         gd_hline_opt(r->left, y, r->right, uhline_func);
         y++;
         r++;
      }
      if (gd_secret_hack_nest_rings) {
         while (y < y_in_max) {
            gd_hline_opt(r->left, y, ri->left-1, uhline_func);
            gd_hline_opt(ri->right+1, y, r->right, uhline_func);
            ++y;
            ++r;
            ++ri;
         }
      } else {
         // make sure we don't have any dropouts/gaps in the ring,
         // since while that might be "correct" it will show our
         // horizontal scan bias

         int xl,xll,xln,xli;
         int xr,xrl,xrn,xri;
         xl=r->left;
         xr=r->right;
         if (r==r0+1) {
            xll=xr+1;
            xrl=xl-1;
         } else {
            xll=(r-1)->left;
            xrl=(r-1)->right;
         }
         if (y_in_max == y_max)
            y_in_max--;
         while (y < y_in_max) {
            xln = (r+1)->left;
            xrn = (r+1)->right;
            xli = ri->left;
            xri = ri->right;
            if (xli < xln)
               xli = xln-1;
            if (xli < xll)
               xli = xll-1;
            if (xri > xrn)
               xri = xrn+1;
            if (xri > xrl)
               xri = xrl+1;
            if (xri <= xli) {
               gd_hline_opt(r->left, y, r->right, uhline_func);
            } else {
               gd_hline_opt(r->left, y, xli, uhline_func);
               gd_hline_opt(xri, y, r->right, uhline_func);
            }
            xll = xl;
            xrl = xr;
            xl = xln;
            xr = xrn;
            y++;
            r++;
            ri++;
         }
      }
      gr_free_temp(r0i);
   }
   while (y < y_max) {
      gd_hline_opt(r->left, y, r->right, uhline_func);
      y++;
      r++;
   }
   gr_free_temp(r0);
   return CLIP_NONE;
}

