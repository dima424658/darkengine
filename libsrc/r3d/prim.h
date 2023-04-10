// $Header: x:/prj/tech/libsrc/r3d/RCS/prim.h 1.12 1998/06/30 18:45:54 buzzard Exp $

#ifndef __PRIM_H
#define __PRIM_H

#include <ctxts.h>
#include <r3ds.h>

EXTERN void r3_set_2d_clip(bool context);
EXTERN void r3_set_color(int color);
EXTERN void r3_set_point_context(int context);
EXTERN void r3_set_line_context(int context);
EXTERN void r3_set_polygon_context(int context);
EXTERN void r3_set_texture(r3s_texture id);
EXTERN void r3_force_texture(r3s_texture id);

// force recalculation of all internal primitve state
EXTERN void r3_set_prim(void);

EXTERN int r3_draw_point(r3s_phandle p);
EXTERN int r3_draw_line(r3s_phandle p0,r3s_phandle p1);
EXTERN int r3_bitmap (r3s_texture bm, r3s_phandle p);

#define r3_draw_poly     (r3d_glob.prim_tab.draw_3d_poly)
#define r3_draw_tmap     (r3d_glob.prim_tab.draw_3d_tmap)
EXTERN BOOL r3d_do_setup;

// flags for draw_point context
#define R3_PT_TRANSLUCENT 1
#define R3_PT_ANTI_ALIAS  2


// flags for draw_line context
#define R3_LN_FLAT             0
#define R3_LN_RGB_GOURAUD      1
#define R3_LN_CLUT_GOURUAD     2
#define R3_LN_PALETTE_GOURAUD  3


// flags for polygon drawing

   // primary drawing mode
#define R3_PL_POLYGON          0
#define R3_PL_TRISTRIP         1
#define R3_PL_TRIFAN           2
#define R3_PL_QUADSTRIP        3

   // lighting mode
#define R3_PL_UNLIT            0
#define R3_PL_GOURAUD          4
#define R3_PL_RGB_GOURAUD      8
#define R3_PL_PALETTE_GOURAUD  12

   // fill modes
#define R3_PL_SOLID             0
#define R3_PL_TRANSLUCENT      16
#define R3_PL_TEXTURE          32
#define R3_PL_TEXTURE_LINEAR   48

   // outline in wireframe as well as draw normally
#define R3_PL_WIRE             64

   // check facing using vertices
#define R3_PL_CHECK_FACING     128

#define R3_PL_MODE_MASK         3
#define R3_PL_LIGHT_MASK       12
#define R3_PL_TEXTURE_MASK     48

#endif // __PRIM_H





