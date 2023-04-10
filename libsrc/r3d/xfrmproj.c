/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/xfrmproj.c $
 * $Revision: 1.9 $
 * $Author: KEVIN $
 * $Date: 1996/10/29 16:11:53 $
 *
 * Transform and projection definitions for projection space
 *
 */


#include <lg.h>
#include <xfrmtab.h>
#include <_xfrm.h>
#include <clipcon_.h>
#include <matrix.h>
#include <clip_.h>

#define USE_ASM
extern void r3_projectspace_code_points(int, r3s_point *); // HACK HACK HACK

void r3_rotate_block_proj(int n, r3s_point *dst, mxs_vector *src)
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

void r3_project_block_proj(int n, r3s_point *p_list)
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

      p->grp.sx = (int)((p->p.x * w) + r3d_glob.x_off);
      p->grp.sy = (int)((p->p.y * w) + r3d_glob.y_off);
      p->grp.w = w;
   }

   if (r3d_clip_mode != R3_NO_CLIP)
      r3_projectspace_code_points(n, p_list);
}

#ifdef USE_ASM
 // assembly declarations
extern void r3_transform_block_proj_clip(int n, r3s_point *dest, mxs_vector *src, mxs_trans *t);
extern void r3_transform_block_proj_noclip(int n, r3s_point *dest, mxs_vector *src, mxs_trans *t);
#endif

void r3_transform_block_proj(int n, r3s_point *dest, mxs_vector *src)
{
#ifdef USE_ASM
   TEST_IN_BLOCK("r3_transform_block_proj");

   if (r3d_clip_mode != R3_CLIP)
      r3_transform_block_proj_noclip(n, dest, src, X2TRANS(&cx.o2c));
   else
      r3_transform_block_proj_clip(n, dest, src, X2TRANS(&cx.o2c));

   if (r3d_clip_mode != R3_NO_CLIP && r3d_num_planes)
      r3_std_code_points(n, dest);
#else
   uchar *cur, *last;

   TEST_IN_BLOCK("r3_transform_block_proj");

   cur = (uchar *)p_list;
   last = cur + n * r3d_glob.cur_stride;

   while (cur < last) {
      r3s_point *dst = (r3s_point *)cur;
      double w;

      mx_trans_mul_vec(&dst->p, X2TRANS(&cx.o2c), src++);
      w = 1.0/dst->p.z; // w is 1/z;
      dst->grp.sx = (int)(dst->p.x * w) + r3d_glob.x_off;
      dst->grp.sy = (int)(dst->p.y * w) + r3d_glob.y_off;
      dst->grp.w = w;
      cur += r3d_glob.cur_stride;
   }

   if (r3d_clip_mode != R3_NO_CLIP)
      r3_projectspace_code_points(n, dest);
#endif
}

r3s_xform_tab r3d_xform_tab_proj = {
   r3_rotate_block_proj,
   r3_project_block_proj,
   r3_transform_block_proj
};

