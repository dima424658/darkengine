// $Header: x:/prj/tech/libsrc/r3d/RCS/primconv.h 1.6 1998/06/18 11:26:46 JAEMZ Exp $

////////  convert between 3d points and 2d points  ////////

EXTERN void r3_phandle_to_vertex_list(int n, r3s_phandle *src, grs_point **vp);
   // convert an array of phandles into an array vertices
   // this automatically reverses the winding in a left-handed space

#define r3_phandle_to_vertex(src)    (&((src)->grp))
   // convert a phandle into a grs_vertex *

EXTERN void r3_reverse_vertex_list(int n, grs_point **src);
   // reverse 2d vertex list by hand

EXTERN void r3_maybe_reverse_vertex_list(int n, grs_point **src);
   // reverse 2d vertex list if we're in a left-handed space

