// $Header: x:/prj/tech/libsrc/r3d/RCS/clipclip.c 1.9 1996/10/29 12:44:47 KEVIN Exp $

// clipping is:
//    off the right is  y < -x
//    off the left is   y > x
//    off the top is    z > x
//    off the bottom is z < -x

//  clipspace clipping

#include <r3ds.h>
#include <clip_.h>
#include <clipcon_.h>
#include <clipoff.h>
#include <_test.h>

void r3_clipspace_code_points(int n, r3s_point *src)
{
   uchar *cur, *last;
   mxs_real nearxxx = r3d_near_plane;

   TEST_IN_BLOCK("r3_clipspace_code_points");

   cur = (uchar *)src;
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      r3s_point *p = (r3s_point *)cur;
      int code = 0;
      mxs_real z = p->p.z;

      cur += r3d_glob.cur_stride;

      if (p->p.x > z) code = R3C_OFF_RIGHT; else code = R3C_NONE;
      if (p->p.x < -z) code |= R3C_OFF_LEFT;

      if (p->p.y < -z) code |= R3C_OFF_TOP;
      if (p->p.y > z) code |= R3C_OFF_BOTTOM;

      if (z < nearxxx) code |= R3C_BEHIND;

      p->ccodes = code;
   }   

   if (r3d_num_planes)
      r3_std_code_points(n, src);
}
