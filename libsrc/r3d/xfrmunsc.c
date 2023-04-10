/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/xfrmunsc.c $
 * $Revision: 1.7 $
 * $Author: KEVIN $
 * $Date: 1996/10/29 12:45:02 $
 *
 * Transform and projection definitions for unscaled space
 *
 */

#include <lg.h>
#include <xfrmtab.h>
#include <_xfrm.h>
#include <clipcon_.h>
#include <matrix.h>

extern void r3_unscaledspace_code_points(int, r3s_point *); // HACK HACK HACK

void r3_rotate_block_unsc(int n,r3s_point *dst,mxs_vector *src)
{
   uchar *cur, *last;

   TEST_IN_BLOCK("r3_rotate_block_proj");

   cur = (uchar *)(&(dst->p));
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      mx_trans_mul_vec((mxs_vector *)cur, X2TRANS(&cx.o2c), src++);
      cur += r3d_glob.cur_stride;
   }
}

void r3_project_block_unsc(int n,r3s_point *p_list)
{
   uchar *cur, *last;

   TEST_IN_BLOCK("r3_project_block_proj");

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

   if (r3d_clip_mode != R3_NO_CLIP)
      r3_unscaledspace_code_points(n, p_list);
}

r3s_xform_tab r3d_xform_tab_unsc = {
   r3_rotate_block_unsc,
   r3_project_block_unsc
};

