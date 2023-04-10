// $Header: x:/prj/tech/libsrc/g2/RCS/g2tm.h 1.2 1998/07/04 13:14:02 PATMAC Exp $

#ifndef __G2TM_H
#define __G2TM_H

#include <dev2d.h>
#include <g2spoint.h>
#include <g2d.h>

EXTERN int g2_map(int n, g2s_point **ppl);
EXTERN int g2_umap(int n, g2s_point **ppl);
EXTERN int g2_per_map(int n, g2s_point **ppl);
EXTERN int g2_per_umap(int n, g2s_point **ppl);
EXTERN int g2_draw_poly_func(int n, g2s_point **ppl);

EXTERN void g2_lin_map_setup(grs_bitmap *bm);
EXTERN void g2_lit_lin_map_setup(grs_bitmap *bm);
EXTERN void g2_lin_umap_setup(grs_bitmap *bm);
EXTERN void g2_lit_lin_umap_setup(grs_bitmap *bm);
EXTERN void g2_per_map_setup(grs_bitmap *bm);
EXTERN void g2_lit_per_map_setup(grs_bitmap *bm);
EXTERN void g2_per_umap_setup(grs_bitmap *bm);
EXTERN void g2_lit_per_umap_setup(grs_bitmap *bm);
EXTERN void g2_poly_setup(grs_bitmap *bm);
EXTERN void g2_spoly_setup(grs_bitmap *bm);
EXTERN void g2_cpoly_setup(grs_bitmap *bm);
EXTERN void g2_upoly_setup(grs_bitmap *bm);
EXTERN void g2_uspoly_setup(grs_bitmap *bm);
EXTERN void g2_ucpoly_setup(grs_bitmap *bm);
EXTERN void g2_poly_setup_null(grs_bitmap *bm);

#endif
