// $Header: x:/prj/tech/libsrc/r3d/RCS/object.h 1.6 1998/06/18 11:26:24 JAEMZ Exp $
// object stack routines

#ifndef __OBJECT_H
#define __OBJECT_H

// Starts an object coordinate system, with no orientation.  What happens
// is the object stack is incremented, and the new transform is
// calculated.  Sets o2c_invalid.  All of them do. 

EXTERN void r3_start_object(mxs_vector *v);
EXTERN void r3_start_object_angles(mxs_vector *v, mxs_angvec *a,r3e_order o);
EXTERN void r3_start_object_trans(mxs_trans *t);

// takes a number representing which axis to rotate about 0=x,1=y,2=z
// real angs are in radian, if you didn't know
EXTERN void r3_start_object_n(mxs_vector *v,mxs_ang ang, int n);

// These next three are useful for retardo-vision objects, and internally
// for bsp objects that are hinged.

#define r3_start_object_x(vecptr,realang) r3_start_object_n((vecptr),(realang),0)
#define r3_start_object_y(vecptr,realang) r3_start_object_n((vecptr),(realang),1)
#define r3_start_object_z(vecptr,realang) r3_start_object_n((vecptr),(realang),2)

// Pops the transform stack

EXTERN void r3_end_object();

// Exotic Transforms

// The other transforms are to go from object to world, on a point by
// point basis.  These are considered pathological, and so, aren't
// implemented as block calls or anything like that.

// Rotate a vector from object space to camera space,
// meaning this is non-orthogonal, beware
EXTERN void r3_rotate_o2c(mxs_vector *dst,mxs_vector *src);

EXTERN void r3_transform_o2w(mxs_vector *dst, mxs_vector *src);
EXTERN void r3_rotate_o2w(mxs_vector *dst, mxs_vector *src);

// this definitely assumes o2w is always orthogonal.  Because it uses
// transpose as inverse.
EXTERN void r3_transform_w2o(mxs_vector *dst, mxs_vector *src);
EXTERN void r3_rotate_w2o(mxs_vector *dst, mxs_vector *src);

#endif // OBJECT










