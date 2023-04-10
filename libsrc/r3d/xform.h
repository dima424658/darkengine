// $Header: x:/prj/tech/libsrc/r3d/RCS/xform.h 1.9 1998/06/18 11:27:29 JAEMZ Exp $
// Transform and projection definitions

#ifndef __XFORM_H
#define __XFORM_H

#include <ctxts.h>


// define to dereference global f pointer
#define R3_XFORM(x) (*((r3d_glob.xform_tab).x))

// The basic set of transformers and projectors are as follows.  The
// project class of functions just projects, stuffing the sx,sy, and w
// values into the 2d part of the point.  The rotate class of functions
// just

// some of these will be function pointers
EXTERN void r3_transform_block(int n,r3s_point dst[],mxs_vector src[]);
#define r3_rotate_block(n,dst,src) (R3_XFORM(rotate_block)((n),(dst),(src)))
#define r3_project_block(n,p) (R3_XFORM(project_block)((n),(p)))
#define r3_transform_block_func R3_XFORM(transform_block)

EXTERN void r3_transform_list(int n,r3s_phandle dst[],mxs_vector *src[]);
EXTERN void r3_rotate_list(int n,r3s_phandle dst[],mxs_vector *src[]);
EXTERN void r3_project_list(int n,r3s_phandle p[]);

#define r3_transform_point(dst,src) r3_transform_block(1,(dst),(src))
#define r3_rotate_point(dst,src) (R3_XFORM(rotate_block)(1,(dst),(src)))
#define r3_project_point(p) (R3_XFORM(project_block)(1,(p)))

// This is the preferred model, especially since every tile renderer is
// fixed anyway it gets you the delta to add to your transformed points. 
// It stuffs the delta into dst.

EXTERN void r3_get_delta(mxs_vector *dst,mxs_vector *src);
EXTERN void r3_copy_add_delta(r3s_point *dst,r3s_point *src,mxs_vector *delta);



#endif // XFORM
