// $Header: x:/prj/tech/libsrc/r3d/RCS/primcon_.h 1.4 1997/05/20 12:09:52 KEVIN Exp $

// primitive context accessors

#include <ctxts.h>

#define prm  (r3d_state.cur_con->prim)

#define r3d_color        prm.color
#define r3d_bm           prm.bm
#define r3d_clip_2d      prm.clip_2d
#define r3d_point_flags  prm.point_flags
#define r3d_line_flags   prm.line_flags
#define r3d_poly_flags   prm.poly_flags
#define r3d_force_texture prm.bm_type
