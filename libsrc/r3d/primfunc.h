// $Header: x:/prj/tech/libsrc/r3d/RCS/primfunc.h 1.6 1998/06/18 11:26:49 JAEMZ Exp $

#ifndef __PRIMFUNC_H
#include <r3ds.h>

  // internal function to set up function pointers
#ifdef DBG_ON
  EXTERN void r3_compute_prim_funcs(int, char *func);
#else
  EXTERN void r3_compute_prim_funcs(int);
  #define r3_compute_prim_funcs(a,b)  r3_compute_prim_funcs(a)
#endif

  // flags for function pointers
#define R3FUNC_3D_POLY    1
#define R3FUNC_3D_TMAP    2
#define R3FUNC_2D_POLY    4
#define R3FUNC_2D_LINE    8
#define R3FUNC_2D_POINT   16

#define R3FUNC_ALL        255

#define R3FUNC_ALL_2D     (R3FUNC_2D_POLY | R3FUNC_2D_LINE | R3FUNC_2D_POINT)
#define R3FUNC_ALL_3D     (R3FUNC_3D_POLY | R3FUNC_3D_TMAP)
#define R3FUNC_ALL_POINT  (R3FUNC_2D_POINT)
#define R3FUNC_ALL_LINE   (R3FUNC_2D_LINE)
#define R3FUNC_ALL_POLY   (R3FUNC_3D_POLY | R3FUNC_3D_TMAP | R3FUNC_2D_POLY)

typedef void (*poly_setup_func)(r3s_texture tm);
EXTERN void r3_setup_prim_funcs(
   poly_setup_func *setup_func_list,
   int (*convert)(int n, r3s_phandle *vl),
   int (*line_func)(r3s_phandle p0, r3s_phandle p1),
   int (*point_func)(r3s_phandle p));

#endif
