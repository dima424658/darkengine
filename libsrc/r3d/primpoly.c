// $Header: x:/prj/tech/libsrc/r3d/RCS/primpoly.c 1.17 1997/10/03 13:03:26 KEVIN Exp $
//
//  polygon drawing functions

#include <dev2d.h> // only for CLIP_ALL (sigh)

#include <r3ds.h>
#include <prim.h>
#include <primtab_.h>
#include <primface.h>
#include <clipprim.h>
#include <primpoly.h>

//  key to naming conventions:
//     check    = check backfacing
//     clip     = apply 3d clipping
//     implicit = r3_draw_poly call--texture not passed in

int r3_check_clip_any(int n, r3s_phandle *vp, r3s_texture bm)
{
   if (r3_check_facing_n(n, vp)) {
      n = r3_raw_clip_polygon(n, vp, &vp);
      if (n) {
         r3_force_texture(bm);
         return r3_convert_func(n, vp);
      }
   }
   return CLIP_ALL;
}

int r3_check_any(int n, r3s_phandle *vp, r3s_texture bm)
{
   if (r3_check_facing_n(n, vp)) {
      r3_force_texture(bm);
      return r3_convert_func(n, vp);
   }
   return CLIP_ALL;
}   

int r3_clip_any(int n, r3s_phandle *vp, r3s_texture bm)
{
   n = r3_raw_clip_polygon(n, vp, &vp);
   if (n) {
      r3_force_texture(bm);
      return r3_convert_func(n, vp);
   }
   return CLIP_ALL;
}

int r3_any(int n, r3s_phandle *vp, r3s_texture bm)
{
   r3_force_texture(bm);
   return r3_convert_func(n, vp);
}

int r3_check_clip_implicit_any(int n, r3s_phandle *vp)
{
   if (r3_check_facing_n(n, vp)) {
      n = r3_raw_clip_polygon(n, vp, &vp);
      if (n)
         return r3_convert_func(n, vp);
   }
   return CLIP_ALL;
}

int r3_check_implicit_any(int n, r3s_phandle *vp)
{
   if (r3_check_facing_n(n, vp))
      return r3_convert_func(n, vp);
   return CLIP_ALL;
}

int r3_clip_implicit_any(int n, r3s_phandle *vp)
{
   n = r3_raw_clip_polygon(n, vp, &vp);
   if (n)
      return r3_convert_func(n, vp);
   return CLIP_ALL;
}

int r3_implicit_any(int n, r3s_phandle *vp)
{
   return r3_convert_func(n, vp);
}
