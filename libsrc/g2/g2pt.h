/// $Header: x:/prj/tech/libsrc/g2/RCS/g2pt.h 1.3 1998/07/02 12:08:09 PATMAC Exp $

#ifndef __G2PT_H
#define __G2PT_H
#include <fix.h>
#include <dev2d.h>
#include <matrixs.h>
#include <r3ds.h>

#ifdef __cplusplus
extern "C" {
#endif
extern double g2pt_tmap_data[9];
extern uchar *g2pt_clut;
extern uchar *g2pt_tluc_table;
extern bool g2pt_span_clip;


extern void g2pt_duv_set_size(int w, int h);
extern void g2pt_calc_uvw_deltas(mxs_vector *pt, mxs_vector *u, mxs_vector *v);

extern void g2pt_poly_perspective(grs_bitmap *b, int n, r3s_phandle *vp, 
                                    bool lit);

extern void g2pt_poly_perspective_uv(grs_bitmap *b, int n, r3s_phandle *vp, 
                                    fix u, fix v, bool lit);

extern void g2pt_poly_affine(grs_bitmap *b, int n, r3s_phandle *vp, bool lit);

extern void g2pt_reset_clip(int x0, int x1, int max_y);
extern void g2pt_post_render_polys(void);

#ifdef __cplusplus
}
#endif
#endif
