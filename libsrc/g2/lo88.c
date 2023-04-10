// $Header: x:/prj/tech/libsrc/g2/RCS/lo88.c 1.7 1998/04/28 14:31:20 KEVIN Exp $
#include <genrast.h>
#include <trirast.h>
#include <g2clip.h>
#include <tmapd.h>
#include <g2tm.h>

extern g2il_func gen_flat8_lit_il;
extern g2raster_func opaque_lit_8to8_raster;
extern void opaque_lit_8to8_il_init(grs_bitmap *bm);
extern void opaque_lit_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *));
extern void opaque_clut_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *));
extern void opaque_clut_8to8_il_set_clut(uchar *clut);

static int (*poly_func)(int n, g2s_point **ppl);
static bool check_clut_enabled = TRUE;
static bool clutting;

static int check_clut(int n, g2s_point **ppl)
{
   fix i_min, i_max;
   int i;
   float scale = g2d_pp.i_scale * 65536.0;


   i_min = i_max = scale * ppl[0]->i;
   for (i=1; i<n; i++) {
      fix test = scale * ppl[i]->i;
      if (test < i_min)
         i_min = test;
      if (i_max < test)
         i_max = test;
   }

   if (fix_int(i_max) != fix_int(i_min)) {  // light
      if (clutting) {                           // need to setup for lighting
         g2d_pp.flags |= PPF_I;
         if (g2d_pp.clip_type == G2C_CLIP_UVW)
            g2d_pp.clip_type = G2C_CLIP_UVWI;
         else
            g2d_pp.clip_type = G2C_CLIP_UVI;
         g2d_pp.poly_func = poly_func;
         opaque_lit_8to8_setup(g2d_pp.bm, NULL);
         clutting = FALSE;
      }
   } else {                                     // clut
      uchar *clut = grd_light_table + (fix_int(i_max) << 8);
      if (!clutting) {                          // need to setup
         ulong fill_save = gr_get_fill_parm();
         gr_set_fill_parm(clut);
         g2d_pp.flags &= ~PPF_I;
         if (g2d_pp.clip_type == G2C_CLIP_UVWI)
            g2d_pp.clip_type = G2C_CLIP_UVW;
         else
            g2d_pp.clip_type = G2C_CLIP_UV;
         opaque_clut_8to8_setup(g2d_pp.bm, NULL);
         gr_set_fill_parm(fill_save);
         clutting = TRUE;
      } else
         opaque_clut_8to8_il_set_clut(clut);    // just change clut
   }
   return poly_func(n, ppl);
}


#pragma off(unreferenced)
void opaque_lit_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;

   if (POW2(bm)) {
      if (check_clut_enabled) {
         poly_func = g2d_pp.poly_func;
         g2d_pp.poly_func = check_clut;
         clutting = FALSE;
      }
      g2d_pp.canvas_row = grd_bm.row;
      g2d_pp.raster_func = opaque_lit_8to8_raster;
      opaque_lit_8to8_il_init(bm);
   } else {
      g2d_pp.raster_func = gen_raster_loop;
      g2d_pp.dy = 1;
      g2d_pp.pix_func = gd_upix8_expose(0,0,0);
      g2d_pp.lpix_func = gd_lpix8_expose(0,0);
      g2d_pp.inner_loop = gen_flat8_lit_il;
   }
}
#pragma on(unreferenced)
