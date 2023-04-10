// $Header: x:/prj/tech/libsrc/r3d/RCS/prim.c 1.12 1997/10/03 13:02:29 KEVIN Exp $
//
//  Context setting for 3D primitives

#include <dev2d.h>

#include <prim.h>
#include <primfunc.h>
#include <primcon_.h>
#include <primtab_.h>

BOOL r3d_do_setup = TRUE;

  // start up a new context
void r3_set_prim(void)
{
   if (r3d_state.cur_con == NULL)
      return;
   r3_compute_prim_funcs(R3FUNC_ALL, "r3_set_prim");
}

  // use 2d clipping for all primitives?
void r3_set_2d_clip(bool context)
{
   if (context != r3d_clip_2d) {
      r3d_clip_2d = context;
      r3_compute_prim_funcs(R3FUNC_ALL_2D, "r3_set_2d_clip");
   }
}

void r3_set_color(int color)
{
   r3d_color = color;
   grd_gc.fcolor = color;
}

void r3_set_point_context(int context)
{
   if (context != r3d_point_flags) {
      r3d_point_flags = context;
      r3_compute_prim_funcs(R3FUNC_ALL_POINT, "r3_set_point_context");
   }
}

void r3_set_line_context(int context)
{
   if (context != r3d_line_flags) {
      r3d_line_flags = context;
      r3_compute_prim_funcs(R3FUNC_ALL_LINE, "r3_set_line_context");
   }
}

void r3_set_polygon_context(int context)
{
   if (context != r3d_poly_flags) {
      r3d_poly_flags = context;
      r3_compute_prim_funcs(R3FUNC_ALL_POLY, "r3_set_polygon_context");
   }
}

// sets 2d context
void r3_force_texture(r3s_texture id)
{
   // store off the bitmap
   r3d_bm = id;
   r3d_do_setup = TRUE;
}

// only sets 2d context if texture has changed.
void r3_set_texture(r3s_texture id)
{
   if (r3d_bm != id)
      r3_force_texture(id);
}

