// $Header: x:/prj/tech/libsrc/g2/RCS/genf16l.c 1.6 1998/05/01 11:09:11 KEVIN Exp $
// generic flat16 linear mappers

#include <tmapd.h>
#include <genrast.h>
#include <trirast.h>

extern g2il_func gen_flat16_il;
extern g2il_func gen_flat16_il_wrap;
extern g2il_func gen_flat16_lit_il;
extern g2il_func gen_flat16_lit_il_wrap;

#pragma off(unreferenced)
void gen_flat16_ulmap_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   pixpal = (void *)(grd_pal16_list[bm->align]);

   if (POW2(bm))
      g2d_pp.inner_loop = gen_flat16_il_wrap;
   else
      g2d_pp.inner_loop = gen_flat16_il;

   g2d_pp.pix_func = gd_upix16_expose(0,0,0);
   g2d_pp.dy = 1;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}

void gen_flat16_lit_ulmap_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   pixpal = (void *)(grd_pal16_list[bm->align]);

   if (POW2(bm))
      g2d_pp.inner_loop = gen_flat16_lit_il_wrap;
   else
      g2d_pp.inner_loop = gen_flat16_lit_il;

   g2d_pp.pix_func = gd_upix_expose(0,0,0);
   g2d_pp.lpix_func = gd_lpix16_expose(0,0);
   g2d_pp.dy = 1;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}
#pragma on(unreferenced)
