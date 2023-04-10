// $Header: x:/prj/tech/libsrc/g2/RCS/grtm.h 1.5 1997/05/16 10:15:38 KEVIN Exp $

#ifndef __GRTM_H
#define __GRTM_H

#include <dev2d.h>
#include <plytyp.h>

extern int gr_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl);
extern void gr_lin_umap(grs_bitmap *bm, int n, grs_vertex **vpl);
extern int gr_clut_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut);
extern void gr_clut_lin_umap(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut);
extern int gr_lit_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl);
extern void gr_lit_lin_umap(grs_bitmap *bm, int n, grs_vertex **vpl);
extern int gr_per_map(grs_bitmap *bm, int n, grs_vertex **vpl);
extern void gr_per_umap(grs_bitmap *bm, int n, grs_vertex **vpl);
extern int gr_clut_per_map(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut);
extern void gr_clut_per_umap(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut);
extern int gr_lit_per_map(grs_bitmap *bm, int n, grs_vertex **vpl);
extern void gr_lit_per_umap(grs_bitmap *bm, int n, grs_vertex **vpl);

extern int gr_rotate_bitmap(grs_bitmap *bm, fixang theta, fix x, fix y);
extern int gr_rotate_bitmap_90(grs_bitmap *bm, int x, int y);
extern int gr_rotate_bitmap_180(grs_bitmap *bm, int x, int y);
extern int gr_rotate_bitmap_270(grs_bitmap *bm, int x, int y);

#define gr_flat8_clut_lin_umap gr_clut_lin_umap
#define gr_blend_lin_umap gr_lin_umap
#define gr_blend_lin_map gr_lin_map

#define gr_set_per_detail_level(l)
#define gr_set_clut_lit_tol(l)

#endif /* !__GRPLY_H */
