// $Header: x:/prj/tech/libsrc/g2/RCS/genfl8l.c 1.10 1998/05/01 11:09:15 KEVIN Exp $
// generic flat8 linear mappers

#include <tmapd.h>
#include <genrast.h>
#include <trirast.h>

extern g2il_func gen_flat8_il;
extern g2il_func gen_flat8_il_wrap;
extern g2il_func gen_flat8_lit_il;
extern g2il_func gen_flat8_lit_il_wrap;

#pragma off(unreferenced)
void gen_flat8_ulmap_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   pixpal = (void *)(grd_pal16_list[bm->align]);

   if (POW2(bm))
      g2d_pp.inner_loop = gen_flat8_il_wrap;
   else
      g2d_pp.inner_loop = gen_flat8_il;

   g2d_pp.pix_func = gd_upix8_expose(0,0,0);
   g2d_pp.dy = 1;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}

void gen_flat8_lit_ulmap_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   if (grd_ltab816_list!=NULL)
      grd_ltab816 = grd_ltab816_list[bm->align];
   else
      pixpal = (void *)(grd_pal16_list[bm->align]);

   if (POW2(bm))
      g2d_pp.inner_loop = gen_flat8_lit_il_wrap;
   else
      g2d_pp.inner_loop = gen_flat8_lit_il;

   g2d_pp.pix_func = gd_upix_expose(0,0,0);
   g2d_pp.lpix_func = gd_lpix8_expose(0,0);
   g2d_pp.dy = 1;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}
#pragma on(unreferenced)
