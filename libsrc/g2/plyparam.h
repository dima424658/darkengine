// $Header: x:/prj/tech/libsrc/g2/RCS/plyparam.h 1.5 1998/04/28 14:29:16 KEVIN Exp $

#ifndef __PLYPARAM_H
#define __PLYPARAM_H

#include <dev2d.h>
#include <plyrast.h>
#include <g2spoint.h>

#define G2C_NUM_POLY_COORD 8

typedef struct g2s_poly_params g2s_poly_params;

typedef void (g2il_func)(g2s_raster *r, g2s_poly_params *p);
typedef void (g2raster_func)(g2s_poly_params *p, int num_scanlines);
typedef void (g2edge_func)(g2s_point *prev, g2s_point *next, g2s_poly_params *p);
typedef int (g2poly_func)(int n, g2s_point **ppl);
typedef void (g2grad_func)(g2s_point *p0, g2s_point *p1, g2s_point *p2, g2s_poly_params *p);

typedef struct g2s_raster_list {
   union {
      uchar *bytes;
      g2s_raster *raster;
   };
} g2s_raster_list;

typedef struct left_edge {
   int x,dx;
   ulong x_frac, dx_frac;
   fix coord_val[G2C_NUM_POLY_COORD];
   fix coord_delta[G2C_NUM_POLY_COORD];
} left_edge;

typedef struct right_edge {
   fix x,dx;
} right_edge;

struct g2s_poly_params {
   g2s_raster_list raster_list;
   int raster_row;
   union {
      uchar *p;
      int y;
   };
   union {
      int canvas_row;
      int dy;
   };
   ulong flags;
   grs_bitmap *bm;
   union {
      gdupix_func *pix_func;
      gdulin_func *hline_func;
   };
   gdlpix_func *lpix_func;
   union { // coordinate gradients wrt canvas x (dc/dx)
      fix dcx[G2C_NUM_POLY_COORD];
      // first five coords are intensity, texture u, texture v, haze, dryness
      struct { 
         fix dix, dux, dvx, dhx, ddx;
      };
   };
   union { // coordinate gradients wrt canvas y (dc/dy)
      fix dcy[G2C_NUM_POLY_COORD]; 
      struct {
         fix diy, duy, dvy, dhy, ddy;
      };
   };
   union { // Scale factor for each coordinate; Applied when calculating gradients.
      float scale[G2C_NUM_POLY_COORD];
      struct {
         float i_scale, u_scale, v_scale, h_scale, d_scale;
      };
   };
   left_edge left;
   right_edge right;
   g2il_func *inner_loop;
   g2raster_func *raster_func;
   g2edge_func *left_edge_func;
   g2edge_func *right_edge_func;
   g2poly_func *poly_func;
   g2grad_func *grad_func;
   int clip_type;
   int ytop, yright, yleft;
};  

#endif
