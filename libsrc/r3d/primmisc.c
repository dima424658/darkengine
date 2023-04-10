// $Header: x:/prj/tech/libsrc/r3d/RCS/primmisc.c 1.11 1997/10/03 13:03:24 KEVIN Exp $

//  3D misc rendering primitives
#include <dev2d.h>
#include <r3ds.h>

#include <primtab_.h>
#include <clipcon_.h>
#include <clipprim.h>
#include <_test.h>

int r3_draw_point(r3s_phandle p)
{
   TEST_IN_BLOCK("r3_draw_point");
   if (r3d_clip_mode != R3_NO_CLIP)
      if (p->ccodes)
         return CLIP_ALL;

   return r3_2d_point_func(p);
}

int r3_draw_line(r3s_phandle p0, r3s_phandle p1)
{
   TEST_IN_BLOCK("r3_draw_line");
   if (r3d_clip_mode != R3_NO_CLIP) {
      r3s_phandle *res, pt[2];
      pt[0] = p0; pt[1] = p1;
      if (!r3_clip_line(pt, &res))
         return CLIP_ALL;
      p0 = res[0];
      p1 = res[1];
   }
   return r3_2d_line_func(p0, p1);
}

#pragma off(unreferenced)
int r3_bitmap (r3s_texture bm, r3s_phandle p)
{
   return 0;
}

int r3_make_bitmap_poly(r3s_texture bm, r3s_phandle p, r3s_phandle *dest)
{
   return 0;
}
#pragma on(unreferenced)
