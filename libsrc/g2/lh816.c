// $Header: x:/prj/tech/libsrc/g2/RCS/lh816.c 1.8 1998/04/28 14:31:18 KEVIN Exp $

#include <g2clip.h>
#include <g2d.h>
#include <g2tm.h>
#include <genrast.h>
#include <trirast.h>
#include <tmapd.h>

extern void light_il(fix i, fix di, int n);
extern void flat16_flat16_opaque_light_il(uchar *dst, uchar *src, int n, uchar *ltab);
extern void flat16_flat8_opaque_inner_loop(uchar *dst, uchar *src, int n);
extern g2il_func trans_lit_haze_8to16_inner_loop;
extern g2il_func opaque_8to8_buffer_il;
extern void opaque_8to8_buffer_il_init(grs_bitmap *bm);

void opaque_lit_haze_8to16_inner_loop(g2s_raster *r, g2s_poly_params *p)
{
   uchar *tmp = tmap_buffer_end-2*r->n;

   opaque_8to8_buffer_il(r, p);
   flat16_flat8_opaque_inner_loop(tmp, tmap_buffer_end - r->n, r->n);
   light_il(r->i, p->dix, r->n);
   flat16_flat16_opaque_light_il(tmp, tmp, r->n, grd_light_table);
   light_il(r->h, p->dhx, r->n);
   flat16_flat16_opaque_light_il(r->p+2*r->x, tmp, r->n, g2d_haze_table);
}

void lit_haze_8to16_setup(grs_bitmap *bm)
{
   g2d_pp.flags = PPF_IUVH;
   g2d_pp.canvas_row = grd_bm.row;
   pixpal = grd_pal16_list[bm->align];

   g2d_pp.bm = bm;
   g2d_pp.i_scale = grd_light_table_size;
   g2d_pp.u_scale = bm->w;
   g2d_pp.v_scale = bm->h;
   g2d_pp.h_scale = g2d_haze_table_size;
   if (bm->flags&BMF_TRANS)
      g2d_pp.inner_loop = trans_lit_haze_8to16_inner_loop;
   else
      g2d_pp.inner_loop = opaque_lit_haze_8to16_inner_loop;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.grad_func = gen_triangle_gradients;
   opaque_8to8_buffer_il_init(bm);
}

void lit_haze_8to16_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   lit_haze_8to16_setup(bm);
   g2_umap(n, vpl);
}

void opaque_lit_haze_8to16_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   lit_haze_8to16_umap(bm, n, vpl);
}

void trans_lit_haze_8to16_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   lit_haze_8to16_umap(bm, n, vpl);
}

void lit_haze_8to16_pmap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   lit_haze_8to16_setup(bm);
   g2d_pp.clip_type = G2C_CLIP_UVWIH;
   g2_per_umap(n, vpl);
}

