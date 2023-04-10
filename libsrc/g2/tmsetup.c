// $Header: x:/prj/tech/libsrc/g2/RCS/tmsetup.c 1.5 1997/11/21 14:01:15 KEVIN Exp $
#include <dev2d.h>
#include <trirast.h>
#include <g2clip.h>
#include <tmapd.h>
#include <g2tm.h>
#include <g2d.h>
#include <icanvas.h>

#define poly_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_UPOLY]))
#define spoly_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_USPOLY]))
#define cpoly_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_UCPOLY]))
#define tmap_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_ULMAP]))
#define pmap_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_UPMAP]))
#define lit_tmap_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_LIT_ULMAP]))
#define lit_pmap_setup ((void (*)(grs_bitmap *))(g2d_canvas_table[G2C_LIT_UPMAP]))

BOOL g2d_use_g2pt = FALSE;

#ifndef SHIP

static bool poly = TRUE;
static bool upoly = TRUE;
static bool spoly = TRUE;
static bool uspoly = TRUE;
#define CHECK_PUNT(poly_type) \
do {                                   \
   if (!poly_type) {                   \
      g2d_pp.poly_func = g2_poly_null; \
      return;                          \
   }                                   \
} while (0)
void g2_set_uspoly_punt(bool punt) {uspoly = !punt;}

#else

#define CHECK_PUNT(poly_type)
void g2_set_uspoly_punt(bool punt) {}

#endif

static int g2_poly_null(int n, g2s_point **vpl)
{
   return CLIP_ALL;
}

void g2_poly_setup_null(grs_bitmap *bm)
{
   g2d_pp.poly_func = g2_poly_null;
}

void g2_poly_setup(grs_bitmap *bm)
{
   CHECK_PUNT(poly);
   g2d_pp.flags = 0;
   g2d_pp.clip_type = G2C_CLIP_NONE;
   g2d_pp.poly_func = g2_map;
   poly_setup(bm);
}

void g2_upoly_setup(grs_bitmap *bm)
{
   CHECK_PUNT(upoly);
   g2d_pp.flags = 0;
   g2d_pp.poly_func = g2_umap;
   poly_setup(bm);
}

void g2_spoly_setup(grs_bitmap *bm)
{
   CHECK_PUNT(spoly);
   g2d_pp.clip_type = G2C_CLIP_I;
   g2d_pp.poly_func = g2_map;
   g2d_pp.flags = PPF_I;
   g2d_pp.i_scale = grd_light_table_size;
   spoly_setup(bm);
}

void g2_uspoly_setup(grs_bitmap *bm)
{
   CHECK_PUNT(uspoly);
   g2d_pp.poly_func = g2_umap;
   g2d_pp.flags = PPF_I;
   g2d_pp.i_scale = grd_light_table_size;
   spoly_setup(bm);
}

void g2_cpoly_setup(grs_bitmap *bm)
{
   g2d_pp.poly_func = g2_poly_null;
}

void g2_ucpoly_setup(grs_bitmap *bm)
{
   g2d_pp.poly_func = g2_poly_null;
}

static void set_bitmap_params(grs_bitmap *bm)
{
   g2d_pp.bm = bm;
   g2d_pp.u_scale = bm->w;
   g2d_pp.v_scale = bm->h;
}

void g2_lin_map_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_UV;
   g2d_pp.clip_type = G2C_CLIP_UV;
   g2d_pp.poly_func = g2_map;
   tmap_setup(bm);
}

void g2_lit_lin_map_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_IUV;
   g2d_pp.clip_type = G2C_CLIP_UVI;
   g2d_pp.poly_func = g2_map;
   g2d_pp.i_scale = grd_light_table_size;
   lit_tmap_setup(bm);
}

void g2_lin_umap_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_UV;
   g2d_pp.poly_func = g2_umap;
   tmap_setup(bm);
}

void g2_lit_lin_umap_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_IUV;
   g2d_pp.poly_func = g2_umap;
   g2d_pp.i_scale = grd_light_table_size;
   lit_tmap_setup(bm);
}

void g2_per_map_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_UV;
   g2d_pp.clip_type = G2C_CLIP_UVW;
   g2d_pp.poly_func = g2_per_map;
   tmap_setup(bm);
}

void g2_lit_per_map_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_IUV;
   g2d_pp.clip_type = G2C_CLIP_UVWI;
   g2d_pp.poly_func = g2_per_map;
   g2d_pp.i_scale = grd_light_table_size;
   lit_tmap_setup(bm);
}

void gen_upmap_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_UV;
   g2d_pp.clip_type = G2C_CLIP_UVW;
   g2d_pp.poly_func = g2_per_umap;
   tmap_setup(bm);
}

void g2_per_umap_setup(grs_bitmap *bm)
{
   pmap_setup(bm);
}

void gen_lit_upmap_setup(grs_bitmap *bm)
{
   set_bitmap_params(bm);
   g2d_pp.flags = PPF_IUV;
   g2d_pp.clip_type = G2C_CLIP_UVWI;
   g2d_pp.poly_func = g2_per_umap;
   g2d_pp.i_scale = grd_light_table_size;
   lit_tmap_setup(bm);
}

void g2_lit_per_umap_setup(grs_bitmap *bm)
{
   lit_pmap_setup(bm);
}

int g2_draw_poly_func(int n, g2s_point **vpl)
{
   return g2d_pp.poly_func(n, vpl);
}
