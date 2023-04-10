// $Header: x:/prj/tech/libsrc/r3d/RCS/primtab_.h 1.5 1997/06/30 16:35:01 KEVIN Exp $

// primitive function table type

#include <ctxts.h>

#define prmtab   (r3d_glob.prim_tab)

#define r3_3d_poly_func       prmtab.draw_3d_poly
#define r3_3d_tmap_func       prmtab.draw_3d_tmap

#define r3_2d_line_func       prmtab.draw_2d_line
#define r3_2d_point_func      prmtab.draw_2d_point
#define r3_2d_poly_setup_func prmtab.draw_2d_tmap
#define r3_convert_func       prmtab.draw_2d_poly
