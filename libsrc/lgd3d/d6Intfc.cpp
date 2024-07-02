#include <types.h>
#include <grs.h>
#include <r3ds.h>
#include <g2spoint.h>
#include <d6States.h>
#include <lgd3d.h>

#include <d3d.h>
#include <d3dtypes.h>
#include <d6Prim.h>

BOOL lgd3d_g_bInitialized = false;


int lgd3d_rgb_indexed_poly(int n, r3s_point **ppl, r3ixs_info *info);
int lgd3d_rgba_indexed_poly(int n, r3s_point **ppl, r3ixs_info *info);
//int lgd3d_indexed_poly(int n, r3s_point **vpl, r3ixs_info *info);
//int lgd3d_indexed_spoly(int n, r3s_point **vpl, r3ixs_info *info);
int lgd3d_indexed_trifan(int n, r3s_point **vpl, r3ixs_info *info);
//int lgd3d_lit_indexed_trifan(int n, r3s_point **vpl, r3ixs_info *info);
//void lgd3d_tmap_setup(grs_bitmap *bm);
//void lgd3d_rgblit_tmap_setup(grs_bitmap *bm);
int lgd3d_rgblit_trifan(int n, r3s_point **ppl);
int lgd3d_rgblit_indexed_trifan(int n, r3s_point **ppl, r3ixs_info *info);
//void lgd3d_rgbalit_tmap_setup(grs_bitmap *bm);
int lgd3d_rgbalit_trifan(int n, r3s_point **ppl);
int lgd3d_rgbalit_indexed_trifan(int n, r3s_point **ppl, r3ixs_info *info);
//void lgd3d_rgbafoglit_tmap_setup(grs_bitmap *bm);
int lgd3d_rgbafoglit_trifan(int n, r3s_point **ppl);
int lgd3d_rgbafoglit_indexed_trifan(int n, r3s_point **ppl, r3ixs_info *info);
//void lgd3d_diffspecular_tmap_setup(grs_bitmap *bm);
int lgd3d_diffspecular_trifan(int n, r3s_point **ppl);
int lgd3d_diffspecular_indexed_trifan(int n, r3s_point **ppl, r3ixs_info *info);

//void lgd3d_lit_tmap_setup(grs_bitmap *bm);
//void lgd3d_poly_setup();
//void lgd3d_spoly_setup();
//void lgd3d_rgb_poly_setup();
int lgd3d_rgb_poly(int n, r3s_point **ppl);
//void lgd3d_rgba_poly_setup();
int lgd3d_rgba_poly(int n, r3s_point **ppl);

void setwbnf(IDirect3DDevice3 *lpDev, long double dvWNear, long double dvWFar);
