#include <clipcon_.h>
#include <primfunc.h>

  // specify whether clipping is on or off
void r3_set_clipmode(r3e_clipmode x)
{
   r3d_clip_mode = x;
   r3_compute_prim_funcs(R3FUNC_ALL_3D, "r3_set_clipmode");
}


r3e_clipmode r3_get_clipmode()
{
   return r3d_clip_mode;
}


// This also stuffs assembly constant, r3d_near,
// since we want to be able to change this dynamically.
// Note that this is not rigorous, since new clip planes
// in world space are NOT also computed here.  Very bad
void r3_set_near_plane(mxs_real x)
{
   extern float r3d_near;

   r3d_near_plane = x;
   r3d_near = x;
}

mxs_real r3_get_near_plane()
{
   return r3d_near_plane;
}


  // do initialization when a new view matrix is specified
void r3_set_clip(void)
{
   r3_compute_prim_funcs(R3FUNC_ALL, "r3_set_clip during context initialization");
}

void r3_set_clip_flags(int flags)
{
   r3d_clip_flags = flags;
}
