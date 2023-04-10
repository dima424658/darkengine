/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/xfrmtab.h $
 * $Revision: 1.6 $
 * $Author: JAEMZ $
 * $Date: 1998/06/18 11:27:33 $
 *
 * Transform and projection definitions
 *
 */

#include <r3ds.h>

#ifndef __XFORMTAB_H
#define __XFORMTAB_H

typedef struct _r3s_xform_tab {
   void (*rotate_block)(int n,r3s_point dst[],mxs_vector src[]);
   void (*project_block)(int n,r3s_point p[]);
   void (*transform_block)(int n,r3s_point dst[],mxs_vector src[]);
} r3s_xform_tab;

EXTERN r3s_xform_tab *r3d_xform_ftabs[];

#endif // __XFORMTAB_H
