// $Header: x:/prj/tech/libsrc/g2/RCS/co88.c 1.2 1998/04/28 14:31:34 KEVIN Exp $
#include <genrast.h>
#include <tmapd.h>
#include <g2tm.h>

extern g2raster_func opaque_clut_8to8_raster;
extern void opaque_clut_8to8_il_init(grs_bitmap *bm, ulong clut);

#pragma off(unreferenced)
void opaque_clut_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   opaque_clut_8to8_il_init(bm, grd_gc.fill_parm);

   g2d_pp.canvas_row = grd_bm.row;
   g2d_pp.raster_func = opaque_clut_8to8_raster;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
}
#pragma on(unreferenced)
