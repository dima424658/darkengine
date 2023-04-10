/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/xfrmglob.c $
 * $Revision: 1.5 $
 * $Author: buzzard $
 * $Date: 1996/10/21 19:51:34 $
 *
 * Transform and projection definitions
 *
 */

#include <xfrmtab.h>
#include <xform.h>

// Function table for xforms
extern r3s_xform_tab r3d_xform_tab_clip;
extern r3s_xform_tab r3d_xform_tab_proj;
extern r3s_xform_tab r3d_xform_tab_unsc;
extern r3s_xform_tab r3d_xform_tab_lin;

// xform table list
r3s_xform_tab *r3d_xform_ftabs[4] = {
   &r3d_xform_tab_clip,
   &r3d_xform_tab_proj,
   &r3d_xform_tab_unsc,
   &r3d_xform_tab_lin
};


// The basic set of transformers and projectors are as follows.  The
// project class of functions just projects, stuffing the sx,sy, and w
// values into the 2d part of the point.  The rotate class of functions
// just

void r3_transform_block(int n,r3s_point dst[],mxs_vector src[])
{
   if (r3_transform_block_func) {
      r3_transform_block_func(n, dst, src);
   } else {
      r3_rotate_block(n,dst,src);
      r3_project_block(n,dst);
   }
}

void r3_transform_list(int n,r3s_phandle dst[],mxs_vector *src[])
{
   int i;
   for (i=0;i<n;++i) {
      r3_rotate_block(1,dst[i],src[i]);
   }
   for (i=0;i<n;++i) {
      r3_project_block(1,dst[i]);
   }
}

void r3_rotate_list(int n,r3s_phandle dst[],mxs_vector *src[])
{
   int i;
   for (i=0;i<n;++i) {
      r3_rotate_block(1,dst[i],src[i]);
   }
}

void r3_project_list(int n,r3s_phandle p[])
{
   int i;
   for (i=0;i<n;++i) {
      r3_project_block(1,p[i]);
   }
}

// This is the preferred model, especially since every tile renderer is
// fixed anyway it gets you the delta to add to your transformed points. 
// It stuffs the delta into dst.

void r3_get_delta(mxs_vector *dst,mxs_vector *src)
{
   dst->x = src->x;
}

// These are only provided for uniqe circumstances,  if you do them all
// the time on the same value, you are lame.  Also, these always do the
// full transformation.

void r3_copy_add_delta(r3s_point *dst,r3s_point *src,mxs_vector *delta)
{
   dst->p.x = src->p.x + delta->x;
}

