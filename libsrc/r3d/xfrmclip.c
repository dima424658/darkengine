/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/xfrmclip.c $
 * $Revision: 1.7 $
 * $Author: KEVIN $
 * $Date: 1996/10/29 12:44:59 $
 *
 * Transform and projection definitions for clipping space
 *
 */


#include <xfrmtab.h>
#include <_xfrm.h>
#include <clipcon_.h>
#include <matrix.h>

extern void r3_clipspace_code_points(int, r3s_point *); // HACK HACK HACK

// go through and transform all the points.
// note that this also works for projection space, and 
// unscaled space, but not linear, I think.
void r3_rotate_block_clip(int n, r3s_point *dst, mxs_vector *src)
{
   uchar *cur, *last;

   TEST_IN_BLOCK("r3_rotate_block_clip");

   cur = (uchar *)(&(dst->p));
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      mx_trans_mul_vec((mxs_vector *)cur, X2TRANS(&cx.o2c), src++);
      cur += r3d_glob.cur_stride;
   }

   if (r3d_clip_mode == R3_CLIP)
      r3_clipspace_code_points(n, dst);
}

// This is only for clipped
void r3_project_block_clip(int n, r3s_point *p_list)
{
   uchar *cur, *last;

   TEST_IN_BLOCK("r3_project_block_clip");

   cur = (uchar *)p_list;
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      r3s_point *p = (r3s_point *)cur;
      double w;

      cur += r3d_glob.cur_stride;

      w = 1.0/p->p.z; // w is 1/z;

      p->grp.sx = (int)((p->p.x * w * r3d_glob.x_prj) + r3d_glob.x_off);
      p->grp.sy = (int)((p->p.y * w * r3d_glob.y_prj) + r3d_glob.y_off);
      p->grp.w = w;
   }
}

r3s_xform_tab r3d_xform_tab_clip = {
   r3_rotate_block_clip,
   r3_project_block_clip
};
