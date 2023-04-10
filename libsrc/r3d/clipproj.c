// $Header: x:/prj/tech/libsrc/r3d/RCS/clipproj.c 1.8 1997/10/01 13:32:10 KEVIN Exp $

#include <lg.h>
#include <dev2d.h>

#include <r3ds.h>
#include <clip_.h>
#include <clipoff.h>
#include <clipcon_.h>
#include <clipprim.h>
#include <_test.h>

#define cur_canv   (r3d_state.cur_con->cnv)

//  for project space, i = -y/x + i_center
//  now, to check the "left" clipping plane,
//  we test i < 0:
//    -y/x + i_center < 0
//    -y/x < -i_center
//    if x > 0
//      -y < -i_center * x
//      y > i_center*x

extern float r3d_fast_z;

void r3_projectspace_code_points(int n, r3s_point *src)
{
   uchar *cur, *last;
   mxs_real near_limit = r3d_fast_z*256;
   fix w = fix_make(cur_canv->bm.w,0);
   fix h = fix_make(cur_canv->bm.h,0);

   TEST_IN_BLOCK("r3_projectspace_code_points");

   cur = (uchar *)src;
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      r3s_point *p = (r3s_point *)cur;
      int code = 0;
      mxs_real z = p->p.z;
      
      cur += r3d_glob.cur_stride;

      if (z > near_limit) {  // hardly ever happens, sigh
         // fast clipping, requires z > 0 and sx,sy didn't overflow
         if (p->grp.sx < 0) code = R3C_OFF_LEFT;
         else if (p->grp.sx > w) code = R3C_OFF_RIGHT;
         else code = 0;

         if (p->grp.sy < 0) code |= R3C_OFF_TOP;
         else if (p->grp.sy > h) code |= R3C_OFF_BOTTOM;
      } else {
         // slow clipping (need two multiplies)
         mxs_real iz = z * r3d_glob.x_clip;
         mxs_real jz = z * r3d_glob.y_clip;
         code = (z < r3d_near_plane) ? R3C_BEHIND : 0;
         if (p->p.x < -iz) code |= R3C_OFF_LEFT;
         if (p->p.x >  iz) code |= R3C_OFF_RIGHT;
         if (p->p.y < -jz) code |= R3C_OFF_TOP;
         if (p->p.y >  jz) code |= R3C_OFF_BOTTOM;
      }
      p->ccodes = code;
      r3d_ccodes_or |= code;
   }

   if (r3d_num_planes)
      r3_std_code_points(n, src);
}
