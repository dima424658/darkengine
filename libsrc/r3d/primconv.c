// $Header: x:/prj/tech/libsrc/r3d/RCS/primconv.c 1.12 1997/10/01 13:32:48 KEVIN Exp $

#include <r3ds.h>
#include <primconv.h>

////////  convert between 3d points and 2d points  ////////

  // convert an array of phandles into an array vertices
  // this automatically reverses the winding in a left-handed space UNIMPLEMENTED
void r3_phandle_to_vertex_list(int n, r3s_phandle *src, grs_point **vp)
{
   int i;
   for (i=0; i < n; ++i)
      vp[i] = r3_phandle_to_vertex(src[i]);
}

#pragma off(unreferenced)

  // reverse 2d vertex list by hand
void r3_reverse_vertex_list(int n, grs_point **src)
{
}

  // reverse 2d vertex list if we're in a left-handed space
void r3_maybe_reverse_vertex_list(int n, grs_point **src)
{
}

#pragma on(unreferenced)

