// $Header: x:/prj/tech/libsrc/g2/RCS/genply.c 1.3 1998/04/28 14:28:46 KEVIN Exp $

#include <tmapd.h>
#include <plyparam.h>
#include <genrast.h>

static void gen_upoly_il(g2s_raster *r, g2s_poly_params *p)
{
   p->hline_func(r->x, r->y, r->x + r->n - 1);
}

void gen_upoly_setup(grs_bitmap *bm)
{
   g2d_pp.inner_loop = gen_upoly_il;
   g2d_pp.hline_func = gd_uhline_expose(0,0,0);
   g2d_pp.dy = 1;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}
