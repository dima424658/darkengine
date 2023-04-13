// $Header: x:/prj/tech/libsrc/r3d/RCS/primtab.h 1.7 1997/10/03 13:03:29 KEVIN Exp $

// primitive function table type

#pragma once

#include <r3ds.h>
typedef struct grs_vertex grs_vertex;

typedef struct {
   int (*draw_3d_poly) (int n, r3s_phandle *vp);
   int (*draw_3d_tmap) (int n, r3s_phandle *vp, r3s_texture bm);
   int (*draw_2d_poly) (int n, r3s_phandle *vp);
   void (*draw_2d_tmap) (r3s_texture bm); // actually, just setup
   int (*draw_2d_line) (r3s_phandle p0, r3s_phandle p1);
   int (*draw_2d_point)(r3s_phandle p0);
} r3s_prim_tab;
