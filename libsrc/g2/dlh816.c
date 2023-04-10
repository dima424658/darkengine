// $Header: x:/prj/tech/libsrc/g2/RCS/dlh816.c 1.5 1998/04/28 14:31:10 KEVIN Exp $

#include <g2d.h>
#include <trirast.h>
#include <plyshell.h>
#include <genrast.h>
#include <tmapd.h>
#include <g2tm.h>

extern void light_il(fix i, fix di, int n);
extern void flat16_flat16_opaque_light_il(uchar *dst, uchar *src, int n, uchar *ltab);
extern g2il_func opaque_8to8_buffer_il;
extern void opaque_8to8_buffer_il_init(grs_bitmap *bm);

static void dry_lit_haze_il(g2s_raster *r, g2s_poly_params *tp)
{
   int i;
   uchar *buffer = tmap_buffer_end - 2*r->n;
   opaque_8to8_buffer_il(r, tp);
   for (i= - r->n; i<0; i++) {
      int index, c;
      uchar *clut;

      c = tmap_buffer_end[i];
      index = c>>6;
      c &= 0x3f;
      clut = g2d_dryness_table_list[index] +
         ((fix_int(r->d)&g2d_dryness_mask_list[index])<<7); 
      tmap_buffer_end[2*i] = clut[2*c];
      tmap_buffer_end[2*i+1] = clut[2*c+1];
      r->d += tp->ddx;
   }
   light_il(r->i, tp->dix, r->n);
   flat16_flat16_opaque_light_il(buffer, buffer, r->n, grd_light_table);
   light_il(r->h, tp->dhx, r->n);
   flat16_flat16_opaque_light_il(r->p + 2*r->x, buffer, r->n, g2d_haze_table);
}

void g2_dry_lit_haze_umap_setup(grs_bitmap *bm)
{
   g2d_pp.flags = PPF_IUVHD;
   g2d_pp.canvas_row = grd_bm.row;
   g2d_pp.bm = bm;
   g2d_pp.i_scale = grd_light_table_size;
   g2d_pp.u_scale = bm->w;
   g2d_pp.v_scale = bm->h;
   g2d_pp.h_scale = g2d_haze_table_size;
   g2d_pp.d_scale = g2d_dryness_table_size;
   g2d_pp.inner_loop = dry_lit_haze_il;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
   g2d_pp.poly_func = g2_umap;
   opaque_8to8_buffer_il_init(bm);
}

void g2_dry_lit_haze_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2_dry_lit_haze_umap_setup(bm);
   g2_umap(n, vpl);
}
