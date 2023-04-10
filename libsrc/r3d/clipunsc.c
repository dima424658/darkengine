// $Header: x:/prj/tech/libsrc/r3d/RCS/clipunsc.c 1.6 1996/10/29 12:44:56 KEVIN Exp $

#include <lg.h>
#include <r3ds.h>
#include <clip_.h>
#include <clipoff.h>
#include <clipcon_.h>
#include <_test.h>

void r3_unscaledspace_code_points(int n, r3s_point *src)
{
   uchar *cur, *last;
   mxs_real nearxxx = r3d_near_plane;

   TEST_IN_BLOCK("r3_clipspace_code_points");

   cur = (uchar *)src;
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      r3s_point *p = (r3s_point *)cur;
      int code;
      mxs_real z = p->p.z;
      mxs_real iz = r3d_glob.x_clip * z;
      mxs_real jz = r3d_glob.y_clip * z;

      cur += r3d_glob.cur_stride;

      if (p->p.z < nearxxx) code = R3C_BEHIND; else code = 0;
      if (p->p.x < -iz) code |= R3C_OFF_LEFT;
      if (p->p.x >  iz) code |= R3C_OFF_RIGHT;
      if (p->p.y < -jz) code |= R3C_OFF_TOP;
      if (p->p.y >  jz) code |= R3C_OFF_BOTTOM;
      p->ccodes = code;
   }

   if (r3d_num_planes)
      r3_std_code_points(n, src);
}
