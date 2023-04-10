/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matrixd.h $
 * $Revision: 1.22 $
 * $Author: JAEMZ $
 * $Date: 1998/06/18 13:07:01 $
 */


#ifndef __MATRIXD_H
#define __MATRIXD_H
#pragma once

#include <matrixds.h>

#ifdef __cplusplus
extern "C"
{
#endif

// In general these babies take pointers to doubles
// (or defined mxd_vector * and mxd_matrix *
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
void mxd_zero_vec(mxds_vector *v);

// create a unit vec in row n
void mxd_unit_vec(mxds_vector *v,int n);

// copy a vec
void mxd_copy_vec(mxds_vector *dest,const mxds_vector *src);

// dest = v1 + v2;
void mxd_add_vec(mxds_vector *dest, const mxds_vector *v1, const mxds_vector *v2);
// v1 += v2;
void mxd_addeq_vec(mxds_vector *v1, const mxds_vector *v2);

// dest = v1-v2
void mxd_sub_vec(mxds_vector *dest, const mxds_vector *v1, const mxds_vector *v2);
// v1 -= v2;
void mxd_subeq_vec(mxds_vector *v1,const mxds_vector *v2);

// dest = s * v
void mxd_scale_vec(mxds_vector *dest,const mxds_vector *v,double s);
// v *= s
void mxd_scaleeq_vec(mxds_vector *v,double s);

// dest = v1 + s * v2
void mxd_scale_add_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2,double s);
// v1 += s * v2
void mxd_scale_addeq_vec(mxds_vector *v1,const mxds_vector *v2,double s);

// basically interpolate between 2 vectors
// dest = v1*(1-s) + v2*(s)
// s=0 gets you v1, basically
void mxd_interpolate_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2,double s);

// dest = v /s
void mxd_div_vec(mxds_vector *dest,const mxds_vector *v,double s);
// v /= s
void mxd_diveq_vec(mxds_vector *v,double s);

// dest = v1 .* v2      // matlab notation
void mxd_elmul_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2);
// v1 .*= v2
void mxd_elmuleq_vec(mxds_vector *v1,const mxds_vector *v2);

// |v1-v2|^2
double mxd_dist2_vec(const mxds_vector *v1,const mxds_vector *v2);

// |v1-v2|
double mxd_dist_vec(const mxds_vector *v1,const mxds_vector *v2);

// |v|^2
double mxd_mag2_vec(const mxds_vector *v);

// |v|
double mxd_mag_vec(const mxds_vector *v);

// dest = v1/|v1|, returns |v1|
double mxd_norm_vec(mxds_vector *dest,const mxds_vector *v);
// v1 /= |v1|, return |v1|
double mxd_normeq_vec(mxds_vector *v1);

// return v1 . v2
double mxd_dot_vec(const mxds_vector *v1,const mxds_vector *v2);

// dest = v1 x v2
void mxd_cross_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2);

// dest = (v1 x v2) / |v1 x v2|, return |v1 x v2|
double mxd_cross_norm_vec(mxds_vector *dest,const mxds_vector *v1, const mxds_vector *v2);

// dest = -v
void mxd_neg_vec(mxds_vector *dest,const mxds_vector *v);
// v = -v
void mxd_negeq_vec(mxds_vector *v);

// print one out
void mxd_prn_vec(const mxds_vector *v);

// A FEW MORE PURE MATRIX OPS

// Gives you the minimum x,y,z in the dest
void mxd_min_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2);
void mxd_mineq_vec(mxds_vector *dest,const mxds_vector *v);

// Gives you the maximum x,y,z in the dst
void mxd_max_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2);
void mxd_maxeq_vec(mxds_vector *dest,const mxds_vector *v);



// PURE MATRIX OPERATIONS
void mxd_zero_mat(mxds_matrix *m);

// m = I
void mxd_identity_mat(mxds_matrix *m);

// dest = src
void mxd_copy_mat(mxds_matrix *dest,const mxds_matrix *src);

// dest = m * s
void mxd_scale_mat(mxds_matrix *dest,const mxds_matrix *m,double s);
// m *= s
void mxd_scaleeq_mat(mxds_matrix *m,double s);

// dest = m/s
void mxd_div_mat(mxds_matrix *dest,const mxds_matrix *m,double s);
// m /= s
void mxd_diveq_mat(mxds_matrix *m,double s);

// return |m|
double mxd_det_mat(const mxds_matrix *m);

// dest = m^t
void mxd_trans_mat(mxds_matrix *dest,const mxds_matrix *m);
// m = m^t
void mxd_transeq_mat(mxds_matrix *m);

// dest = m^-1, returns false if matrix degenerate
bool mxd_inv_mat(mxds_matrix *dest,const mxds_matrix *m);
// m = m^-1
bool mxd_inveq_mat(mxds_matrix *m);

// normalize each column of m
void mxd_normcol_mat(mxds_matrix *dest,const mxds_matrix *m);
void mxd_normcoleq_mat(mxds_matrix *m);

// normalize each row of m
void mxd_normrow_mat(mxds_matrix *dest,const mxds_matrix *m);
void mxd_normroweq_mat(mxds_matrix *m);

// sanitize the matrix, make |m| = 1 no matter what,
// trying to preserve geometry
bool mxd_sanitize_mat(mxds_matrix *dest,const mxds_matrix *m,double tol);
bool mxd_sanitizeeq_mat(mxds_matrix *m,double tol);

// dest = m1 x m2
void mxd_mul_mat(mxds_matrix *dest,const mxds_matrix *m1,const mxds_matrix *m2);
// m1 = m1 x m2
void mxd_muleq_mat(mxds_matrix *m1,const mxds_matrix *m2);

// dest = m1^t x m2
void mxd_tmul_mat(mxds_matrix *dest,const mxds_matrix *m1,const mxds_matrix *m2);
// m2 = m1^t x m2
void mxd_tmuleq_mat(const mxds_matrix *m1,mxds_matrix *m2);

// dest = m1 x m2^t
void mxd_mult_mat(mxds_matrix *dest,const mxds_matrix *m1,const mxds_matrix *m2);


// dest = m1 .x m2
void mxd_elmul_mat(mxds_matrix *dest,const mxds_matrix *m1,const mxds_matrix *m2);
// m1 .x= m2
void mxd_elmuleq_mat(mxds_matrix *m1,const mxds_matrix *m2);

// swap rows
void mxd_swaprow_mat(mxds_matrix *dest,const mxds_matrix *m1,int n1,int n2);
void mxd_swaproweq_mat(mxds_matrix *m1,int n1,int n2);
// swap cols
void mxd_swapcol_mat(mxds_matrix *dest,const mxds_matrix *m1,int n1,int n2);
void mxd_swapcoleq_mat(mxds_matrix *m1,int n1,int n2);

// print one out
void mxd_prn_mat(const mxds_matrix *m);

// VECTOR ANGLE OPS

// Rotates a vector about the appropriate axis.
// Put in for Sean.  With love, Jaemz
// Assumes src and dest are different
void mxd_rot_x_vec(mxds_vector *dst,const mxds_vector *src,mxs_ang ang);
void mxd_rot_y_vec(mxds_vector *dst,const mxds_vector *src,mxs_ang ang);
void mxd_rot_z_vec(mxds_vector *dst,const mxds_vector *src,mxs_ang ang);

// MATRIX ANGLE OPS

// convert to and from rads
mxs_ang mxd_rad2ang(double rad);
double mxd_ang2rad(mxs_ang ang);

// convert to and from degs
mxs_ang mxd_deg2ang(double deg);
double mxd_ang2deg(double ang);


// get the sin and cosine out of a mxs_ang
// anything else, get it yourself.  Weenie
double mxd_sin(mxs_ang ang);
double mxd_cos(mxs_ang ang);
void mxd_sincos(mxs_ang ang,double *s, double *c);

// create rotation matrices to spin about that axis by ang
void mxd_mk_rot_x_mat(mxds_matrix *m,mxs_ang ang);
void mxd_mk_rot_y_mat(mxds_matrix *m,mxs_ang ang);
void mxd_mk_rot_z_mat(mxds_matrix *m,mxs_ang ang);

// create rotation matrix to spin about arbitrary vector by ang
void mxd_mk_rot_vec_mat(mxds_matrix *m, const mxds_vector *v, mxs_ang ang);
// create rotation matrix to take x-axis to unit vecctor v
void mxd_mk_move_x_mat(mxds_matrix *m, const mxds_vector *v);

// rotate matrix about that axis in its own frame by ang
void mxd_rot_x_mat(mxds_matrix *dest,const mxds_matrix *m,mxs_ang ang);
void mxd_rot_y_mat(mxds_matrix *dest,const mxds_matrix *m,mxs_ang ang);
void mxd_rot_z_mat(mxds_matrix *dest,const mxds_matrix *m,mxs_ang ang);

// and now, lovely rad versions of the above
void mxd_mk_rot_x_mat_rad(mxds_matrix *m,double ang);
void mxd_mk_rot_y_mat_rad(mxds_matrix *m,double ang);
void mxd_mk_rot_z_mat_rad(mxds_matrix *m,double ang);

// rotate matrix about that axis in its own frame by ang
void mxd_rot_x_mat_rad(mxds_matrix *dest,const mxds_matrix *m,double ang);
void mxd_rot_y_mat_rad(mxds_matrix *dest,const mxds_matrix *m,double ang);
void mxd_rot_z_mat_rad(mxds_matrix *dest,const mxds_matrix *m,double ang);

// Create an orientation matrix from an angvec, using traditional
// order of bank around x, pitch around y, heading around z,
// all using the right hand rule
void mxd_ang2mat(mxds_matrix *dest,const mxs_angvec *a);

// Convert a matrix into angles, using the standard
// angle order of roll around x, pitch around y, heading around z
void mxd_mat2ang( mxs_angvec* dest, const mxds_matrix* m);

// just like the above, but radians
void mxd_mat2rad(mxds_vector *dest,const mxds_matrix *m);

// also just like the above, also in radians;
void mxd_rad2mat(mxds_matrix *dest,const mxds_vector *a);

// Print out angvec in hex.  10000 = 2pi
void mxd_prn_angvec(const mxs_angvec *a);


// MIXED MATRIX AND VECTOR OPS

// element wise mul of m by v to make different coordinate
// system
// dest = M .* v
void mxd_mat_elmul_vec(mxds_matrix *dest,const mxds_matrix *m,const mxds_vector *v);
// M .*= v
void mxd_mat_elmuleq_vec(mxds_matrix *m,const mxds_vector *v);

// dest = v^t .* M
void mxd_mat_eltmul_vec(mxds_matrix *dest,const mxds_matrix *m,const mxds_vector *v);
// M .*= v^t
void mxd_mat_eltmuleq_vec(mxds_matrix *m,const mxds_vector *v);

// dest = M x v
void mxd_mat_mul_vec(mxds_vector *dest,const mxds_matrix *m,const mxds_vector *v);
// v x= M, this is idiotic, if anyone uses it, I'll kill them
void mxd_mat_muleq_vec(const mxds_matrix *m,mxds_vector *v);

// dest = M^t x v, this is for multing by inverse if unit
void mxd_mat_tmul_vec(mxds_vector *dest,const mxds_matrix *m, const mxds_vector *v);
// v x= M^t
void mxd_mat_tmuleq_vec(const mxds_matrix *m,mxds_vector *v);

// TRANSFORM OPS

// identity transform
void mxd_identity_trans(mxds_trans *t);

// copy transform
void mxd_copy_trans(mxds_trans *dest,const mxds_trans *src);

// invert transform that is unitary, dest != src
void mxd_transpose_trans(mxds_trans *dest,const mxds_trans *src);

// multiply 2 transforms
// dest = t1 * t2
void mxd_mul_trans(mxds_trans *dest,const mxds_trans *t1,const mxds_trans *t2);

// transpose pre multiply 2 transforms
// for instance, putting view onto a stack
// dest = t1^t * t2
void mxd_tmul_trans(mxds_trans *dest,const mxds_trans *t1,const mxds_trans *t2);

// transpose post multiply 2 transforms
// for instance, putting view onto a stack
// dest = t1 * t2^2
void mxd_mult_trans(mxds_trans *dest,const mxds_trans *t1,const mxds_trans *t2);

// multiply tform by vector
// dest = t * v
void mxd_trans_mul_vec(mxds_vector *dest,const mxds_trans *t,const mxds_vector *v);

// inverse multiply tform by vector
// dest = t^t * v
void mxd_trans_tmul_vec(mxds_vector *dest,const mxds_trans *t,const mxds_vector *v);

// print one out
void mxd_prn_trans(const mxds_trans *t);


#ifdef __cplusplus
};
#endif

#endif /* __MATRIXD_H */




























