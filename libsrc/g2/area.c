// $Header: x:/prj/tech/libsrc/g2/RCS/area.c 1.4 1998/04/28 14:31:30 KEVIN Exp $

#include <g2d.h>
#include <genrast.h>
#include <plyshell.h>

void area_inner_loop(g2s_raster *r, g2s_poly_params *p)
{
   p->dix += r->n;      // update area
}

int g2_poly_area(int n, g2s_point **vpl)
{
   g2s_poly_params tp;

   tp.flags = 0;
   tp.dy = 1;
   tp.dix = 0;          // save area here
   tp.inner_loop = area_inner_loop;
   tp.raster_func = gen_raster_loop;
   tp.right_edge_func = gen_right_edge;
   tp.left_edge_func = gen_left_edge;
   tp.grad_func = gen_triangle_gradients;
   g2_trifan(&tp, n, vpl);
   return tp.dix;
}
