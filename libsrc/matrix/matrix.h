/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matrix.h $
 * $Revision: 1.25 $
 * $Author: buzzard $
 * $Date: 1998/10/26 23:51:23 $
 */


#ifndef __MATRIX_H
#define __MATRIX_H
#pragma once

#include <matrixs.h>

#ifdef __cplusplus
extern "C"
{
#endif

// In general these babies take pointers to mxs_reals
// (or defined mx_vector * and mx_matrix *
// normal vecs are 3, mats are 3x3
// note that rather than intel I favor src -> dest
// self modifiers have the "eq" post on the op.
// in don't make dest = src, use the eq function
// el means element wise.
// coordinates are zero indexed
// some functions will be implemented as macros, esp
// the eq or non-eq versions of some stuff

// These two constructors are mostly for use in constructing matrices and vectors from
// constants, but it should be fast and pretty much general purpose.  Since these are
// macros beware of side effects


// PURE VECTOR OPERATIONS

// v = 0
void mx_zero_vec(mxs_vector *v);

// create a unit vec in row n
void mx_unit_vec(mxs_vector *v,int n);

// copy a vec
void mx_copy_vec(mxs_vector *dest,const mxs_vector *src);

// dest = v1 + v2;
void mx_add_vec(mxs_vector *dest, const mxs_vector *v1, const mxs_vector *v2);
// v1 += v2;
void mx_addeq_vec(mxs_vector *v1, const mxs_vector *v2);

// dest = v1-v2
void mx_sub_vec(mxs_vector *dest, const mxs_vector *v1, const mxs_vector *v2);
// v1 -= v2;
void mx_subeq_vec(mxs_vector *v1,const mxs_vector *v2);

// dest = s * v
void mx_scale_vec(mxs_vector *dest,const mxs_vector *v,mxs_real s);
// v *= s
void mx_scaleeq_vec(mxs_vector *v,mxs_real s);

// dest = v1 + s * v2
void mx_scale_add_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2,mxs_real s);
// v1 += s * v2
void mx_scale_addeq_vec(mxs_vector *v1,const mxs_vector *v2,mxs_real s);

// basically interpolate between 2 vectors
// dest = v1*(1-s) + v2*(s)
// s=0 gets you v1, basically
void mx_interpolate_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2,mxs_real s);

// dest = v /s
void mx_div_vec(mxs_vector *dest,const mxs_vector *v,mxs_real s);
// v /= s
void mx_diveq_vec(mxs_vector *v,mxs_real s);

// dest = v1 .* v2      // matlab notation
void mx_elmul_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2);
// v1 .*= v2
void mx_elmuleq_vec(mxs_vector *v1,const mxs_vector *v2);

// |v1-v2|^2
mxs_real mx_dist2_vec(const mxs_vector *v1,const mxs_vector *v2);

// |v1-v2|
mxs_real mx_dist_vec(const mxs_vector *v1,const mxs_vector *v2);

// |v|^2
mxs_real mx_mag2_vec(const mxs_vector *v);

// |v|
mxs_real mx_mag_vec(const mxs_vector *v);

// dest = v1/|v1|, returns |v1|
mxs_real mx_norm_vec(mxs_vector *dest,const mxs_vector *v);
// v1 /= |v1|, return |v1|
mxs_real mx_normeq_vec(mxs_vector *v1);

// return v1 . v2
#ifdef _MSC_VER
__inline mxs_real mx_dot_vec(const mxs_vector *v1, const mxs_vector *v2)
{
   return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}
#else
mxs_real mx_dot_vec(const mxs_vector *v1,const mxs_vector *v2);
#endif

// dest = v1 x v2
void mx_cross_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2);

// dest = (v1 x v2) / |v1 x v2|, return |v1 x v2|
mxs_real mx_cross_norm_vec(mxs_vector *dest,const mxs_vector *v1, const mxs_vector *v2);

// dest = -v
void mx_neg_vec(mxs_vector *dest,const mxs_vector *v);
// v = -v
void mx_negeq_vec(mxs_vector *v);

// print one out
void mx_prn_vec(const mxs_vector *v);

// A FEW MORE PURE MATRIX OPS

// Gives you the minimum x,y,z in the dest
void mx_min_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2);
void mx_mineq_vec(mxs_vector *dest,const mxs_vector *v);

// Gives you the maximum x,y,z in the dst
void mx_max_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2);
void mx_maxeq_vec(mxs_vector *dest,const mxs_vector *v);

// If the delta of any element is greater than eps, return false
// Has fast rejection
bool mx_is_identical(const mxs_vector *v1,const mxs_vector *v2,float eps);

// PURE MATRIX OPERATIONS
void mx_zero_mat(mxs_matrix *m);

// m = I
void mx_identity_mat(mxs_matrix *m);

// dest = src
void mx_copy_mat(mxs_matrix *dest,const mxs_matrix *src);

// dest = m * s
void mx_scale_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_real s);
// m *= s
void mx_scaleeq_mat(mxs_matrix *m,mxs_real s);

// dest = m/s
void mx_div_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_real s);
// m /= s
void mx_diveq_mat(mxs_matrix *m,mxs_real s);

// return |m|
mxs_real mx_det_mat(const mxs_matrix *m);

// dest = m^t
void mx_trans_mat(mxs_matrix *dest,const mxs_matrix *m);
// m = m^t
void mx_transeq_mat(mxs_matrix *m);

// dest = m^-1, returns false if matrix degenerate
bool mx_inv_mat(mxs_matrix *dest,const mxs_matrix *m);
// m = m^-1
bool mx_inveq_mat(mxs_matrix *m);

// normalize each column of m
void mx_normcol_mat(mxs_matrix *dest,const mxs_matrix *m);
void mx_normcoleq_mat(mxs_matrix *m);

// normalize each row of m
void mx_normrow_mat(mxs_matrix *dest,const mxs_matrix *m);
void mx_normroweq_mat(mxs_matrix *m);

// sanitize the matrix, make |m| = 1 no matter what,
// trying to preserve geometry
bool mx_sanitize_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_real tol);
bool mx_sanitizeeq_mat(mxs_matrix *m,mxs_real tol);

// dest = m1 x m2
void mx_mul_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2);
// m1 = m1 x m2
void mx_muleq_mat(mxs_matrix *m1,const mxs_matrix *m2);

// dest = m1^t x m2
void mx_tmul_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2);
// m2 = m1^t x m2
void mx_tmuleq_mat(const mxs_matrix *m1,mxs_matrix *m2);

// dest = m1 x m2^t
void mx_mult_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2);


// dest = m1 .x m2
void mx_elmul_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2);
// m1 .x= m2
void mx_elmuleq_mat(mxs_matrix *m1,const mxs_matrix *m2);

// swap rows
void mx_swaprow_mat(mxs_matrix *dest,const mxs_matrix *m1,int n1,int n2);
void mx_swaproweq_mat(mxs_matrix *m1,int n1,int n2);
// swap cols
void mx_swapcol_mat(mxs_matrix *dest,const mxs_matrix *m1,int n1,int n2);
void mx_swapcoleq_mat(mxs_matrix *m1,int n1,int n2);

// print one out
void mx_prn_mat(const mxs_matrix *m);

// VECTOR ANGLE OPS

// Rotates a vector about the appropriate axis.
// Put in for Sean.  With love, Jaemz
// Assumes src and dest are different
void mx_rot_x_vec(mxs_vector *dst,const mxs_vector *src,mxs_ang ang);
void mx_rot_y_vec(mxs_vector *dst,const mxs_vector *src,mxs_ang ang);
void mx_rot_z_vec(mxs_vector *dst,const mxs_vector *src,mxs_ang ang);

// MATRIX ANGLE OPS

// convert to and from rads
mxs_ang mx_rad2ang(mxs_real rad);
mxs_real mx_ang2rad(mxs_ang ang);

// convert to and from degs
mxs_ang mx_deg2ang(mxs_real deg);
mxs_real mx_ang2deg(mxs_real ang);


// get the sin and cosine out of a mxs_ang
// anything else, get it yourself.  Weenie
mxs_real mx_sin(mxs_ang ang);
mxs_real mx_cos(mxs_ang ang);
void mx_sincos(mxs_ang ang,mxs_real *s, mxs_real *c);

// create rotation matrices to spin about that axis by ang
void mx_mk_rot_x_mat(mxs_matrix *m,mxs_ang ang);
void mx_mk_rot_y_mat(mxs_matrix *m,mxs_ang ang);
void mx_mk_rot_z_mat(mxs_matrix *m,mxs_ang ang);

// create rotation matrix to spin about arbitrary vector by ang
void mx_mk_rot_vec_mat(mxs_matrix *m, const mxs_vector *v, mxs_ang ang);
// create rotation matrix to take x-axis to unit vecctor v
void mx_mk_move_x_mat(mxs_matrix *m, const mxs_vector *v);

// rotate matrix about that axis in its own frame by ang
void mx_rot_x_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_ang ang);
void mx_rot_y_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_ang ang);
void mx_rot_z_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_ang ang);

// and now, lovely rad versions of the above
void mx_mk_rot_x_mat_rad(mxs_matrix *m,float ang);
void mx_mk_rot_y_mat_rad(mxs_matrix *m,float ang);
void mx_mk_rot_z_mat_rad(mxs_matrix *m,float ang);

// rotate matrix about that axis in its own frame by ang
void mx_rot_x_mat_rad(mxs_matrix *dest,const mxs_matrix *m,float ang);
void mx_rot_y_mat_rad(mxs_matrix *dest,const mxs_matrix *m,float ang);
void mx_rot_z_mat_rad(mxs_matrix *dest,const mxs_matrix *m,float ang);

// Create an orientation matrix from an angvec, using traditional
// order of bank around x, pitch around y, heading around z,
// all using the right hand rule
void mx_ang2mat(mxs_matrix *dest,const mxs_angvec *a);

// Convert a matrix into angles, using the standard
// angle order of roll around x, pitch around y, heading around z
void mx_mat2ang( mxs_angvec* dest, const mxs_matrix* m);

// just like the above, but radians
void mx_mat2rad(mxs_vector *dest,const mxs_matrix *m);

// also just like the above, also in radians;
void mx_rad2mat(mxs_matrix *dest,const mxs_vector *a);

// Print out angvec in hex.  10000 = 2pi
void mx_prn_angvec(const mxs_angvec *a);


// MIXED MATRIX AND VECTOR OPS

// element wise mul of m by v to make different coordinate
// system
// dest = M .* v
void mx_mat_elmul_vec(mxs_matrix *dest,const mxs_matrix *m,const mxs_vector *v);
// M .*= v
void mx_mat_elmuleq_vec(mxs_matrix *m,const mxs_vector *v);

// dest = v^t .* M
void mx_mat_eltmul_vec(mxs_matrix *dest,const mxs_matrix *m,const mxs_vector *v);
// M .*= v^t
void mx_mat_eltmuleq_vec(mxs_matrix *m,const mxs_vector *v);

// dest = M x v
void mx_mat_mul_vec(mxs_vector *dest,const mxs_matrix *m,const mxs_vector *v);
// v x= M, this is idiotic, if anyone uses it, I'll kill them
void mx_mat_muleq_vec(const mxs_matrix *m,mxs_vector *v);

// dest = M^t x v, this is for multing by inverse if unit
void mx_mat_tmul_vec(mxs_vector *dest,const mxs_matrix *m, const mxs_vector *v);
// v x= M^t
void mx_mat_tmuleq_vec(const mxs_matrix *m,mxs_vector *v);

// TRANSFORM OPS

// identity transform
void mx_identity_trans(mxs_trans *t);

// copy transform
void mx_copy_trans(mxs_trans *dest,const mxs_trans *src);

// invert transform that is unitary, dest != src
void mx_transpose_trans(mxs_trans *dest,const mxs_trans *src);

// multiply 2 transforms
// dest = t1 * t2
void mx_mul_trans(mxs_trans *dest,const mxs_trans *t1,const mxs_trans *t2);

// transpose pre multiply 2 transforms
// for instance, putting view onto a stack
// dest = t1^t * t2
void mx_tmul_trans(mxs_trans *dest,const mxs_trans *t1,const mxs_trans *t2);

// transpose post multiply 2 transforms
// for instance, putting view onto a stack
// dest = t1 * t2^2
void mx_mult_trans(mxs_trans *dest,const mxs_trans *t1,const mxs_trans *t2);

// multiply tform by vector
// dest = t * v
void mx_trans_mul_vec(mxs_vector *dest,const mxs_trans *t,const mxs_vector *v);

// inverse multiply tform by vector
// dest = t^t * v
void mx_trans_tmul_vec(mxs_vector *dest,const mxs_trans *t,const mxs_vector *v);

// print one out
void mx_prn_trans(const mxs_trans *t);

// Utility
// Create a matrix from an unormalized vector v, which points
// in the direction of something (x vector) and takes a z vector.
// returns |v|
float mx_mat_look_at_z(mxs_matrix *dest,const mxs_vector *v,const mxs_vector *z);

// Same as above but assumes z vector (0,0,1), ie, bank of zero.
float mx_mat_look_at(mxs_matrix *dest,const mxs_vector *v);


#ifdef __cplusplus
};
#endif

#endif /* __MATRIX_H */