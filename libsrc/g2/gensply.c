// $Header: x:/prj/tech/libsrc/g2/RCS/gensply.c 1.4 1998/04/28 14:28:47 KEVIN Exp $

#include <tmapd.h>
#include <plyparam.h>
#include <genrast.h>

static void gen_uspoly_il(g2s_raster *r, g2s_poly_params *p)
{
   int x = r->x;
   int y = r->y;
   fix i = r->i;
   int xf = r->x + r->n;
   int c = grd_gc.fcolor;
   do {
      p->pix_func(p->lpix_func(i, c), x, y);
      i+=p->dix;
      x++;
   } while (x<xf);
}

void gen_uspoly_setup(grs_bitmap *bm)
{
   if ((grd_bpp > 8) && (grd_ltab816_list!=NULL))
   {
      grd_ltab816 = grd_ltab816_list[0];
      g2d_pp.lpix_func = gd_lpix8_expose(0,0);
   } else
      g2d_pp.lpix_func = gd_lpix_expose(0,0);

   g2d_pp.inner_loop = gen_uspoly_il;
   g2d_pp.pix_func = gd_upix_expose(0,0,0);
   g2d_pp.dy = 1;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}
