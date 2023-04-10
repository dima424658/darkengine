// $Header: x:/prj/tech/libsrc/g2/RCS/fl8sply.c 1.2 1998/04/28 14:31:36 KEVIN Exp $

#include <tmapd.h>
#include <plyparam.h>
#include <genrast.h>

static void flat8_uspoly_il(g2s_raster *r, g2s_poly_params *x)
{
   fix i = r->i;
   fix di = x->dix;
   uchar *p = r->p + r->x;
   uchar *end_p = p + r->n;
   uchar *lt = grd_light_table + grd_gc.fcolor;
   do {
      *p++ = lt[(i >> 8) & 0xff00];
      i+=di;
   } while (p < end_p);
}

void flat8_uspoly_setup(grs_bitmap *bm)
{
   g2d_pp.inner_loop = flat8_uspoly_il;
   g2d_pp.pix_func = gd_upix_expose(0,0,0);
   g2d_pp.lpix_func = gd_lpix_expose(0,0);
   g2d_pp.canvas_row = grd_bm.row;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}
