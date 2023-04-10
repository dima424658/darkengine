// $Header: x:/prj/tech/libsrc/g2/RCS/o88.c 1.4 1998/04/28 14:31:37 KEVIN Exp $
#include <genrast.h>
#include <tmapd.h>
#include <g2tm.h>

extern g2il_func gen_flat8_il;
extern g2raster_func opaque_8to8_raster;
extern void opaque_8to8_il_init(grs_bitmap *bm);

#pragma off(unreferenced)
void opaque_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;

   if (POW2(bm)) {
      g2d_pp.grad_func = uv_triangle_gradients;
      g2d_pp.canvas_row = grd_bm.row;
      g2d_pp.raster_func = opaque_8to8_raster;
      opaque_8to8_il_init(bm);
   } else {
      g2d_pp.grad_func = gen_triangle_gradients;
      g2d_pp.inner_loop = gen_flat8_il;
      g2d_pp.pix_func = gd_upix8_expose(0,0,0);
      g2d_pp.dy = 1;
      g2d_pp.raster_func = gen_raster_loop;
   }
}
#pragma on(unreferenced)

void opaque_8to8_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2_lin_umap_setup(bm);
   g2d_pp.poly_func(n, vpl);
}

void opaque_8to8_pmap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2_per_umap_setup(bm);
   g2d_pp.poly_func(n, vpl);
}
