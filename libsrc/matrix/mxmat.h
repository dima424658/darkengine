/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/matrix/RCS/mxmat.h 1.4 1998/09/11 12:16:19 MAT Exp $

#ifndef __MXMAT_H
#define __MXMAT_H
#pragma once

// This is the file to include to get the C++ sMxVectors.

#include <mxmats.h>
#include <mxdmats.h>
#include <mxangs.h>

// Vector Operators

// This needs both types to define the copy constructors
inline sMxVector::sMxVector(const sMxdVector &v) 
{x=v.x;y=v.y;z=v.z;}

inline sMxVector &sMxVector::operator=(const sMxdVector &v) 
{x=v.x;y=v.y;z=v.z; return *this; }

// Vector Matrix functions:
// dest = M x v
inline sMxVector &sMxVector::Mul(const sMxMatrix &m,const sMxVector &v)
{mx_mat_mul_vec(this,&m,&v); return *this;}
   
// v x= M, this is idiotic, if anyone uses it, I'll kill them        
inline sMxVector &sMxVector::MulEq(const sMxMatrix &m)
{ mx_mat_muleq_vec(&m,this);return *this;}

// dest = M^t x v, this is for multing by inverse if unit
inline sMxVector &sMxVector::TransMul(const sMxMatrix &m,const sMxVector &v)
{mx_mat_tmul_vec(this,&m,&v); return *this;}
// v x= M^t
inline sMxVector &sMxVector::TransMulEq(const sMxMatrix &m)
{ mx_mat_tmuleq_vec(&m,this);return *this;}

// Vector Transform functions:
// multiply tform by vector
// this = t * v
inline sMxVector &sMxVector::Mul(const sMxTrans &t,const sMxVector &v)
{mx_trans_mul_vec(this,&t,&v); return *this;}
   
// inverse multiply tform by vector
// this = t^t * v
inline sMxVector &sMxVector::TransMul(const sMxTrans &t,const sMxVector &v)
{mx_trans_tmul_vec(this,&t,&v); return *this;}


// Matrix Operators with AngVecs and RadVecs

inline sMxMatrix::sMxMatrix(const sMxAngVec &a) 
{ mx_ang2mat(this,&a);}
   
inline sMxMatrix::sMxMatrix(const sMxRadVec &r) 
{ mx_rad2mat(this,&r);}

inline sMxMatrix &sMxMatrix::Set(const sMxAngVec &a) 
{ mx_ang2mat(this,&a); return *this;}
   
inline sMxMatrix &sMxMatrix::Set(const sMxRadVec &r) 
{ mx_rad2mat(this,&r); return *this;}


// Vector Angle
inline sMxVector &sMxVector::RotX(const sMxVector &v,const sMxAng &a)
{ mx_rot_x_vec(this,&v,a.a); return *this;}
inline sMxVector &sMxVector::RotY(const sMxVector &v,const sMxAng &a)
{ mx_rot_y_vec(this,&v,a.a); return *this;}
inline sMxVector &sMxVector::RotZ(const sMxVector &v,const sMxAng &a)
{ mx_rot_z_vec(this,&v,a.a); return *this;}



// Matrix angle ops
// Creates a matrix rotated about that axis by angle amount
inline sMxMatrix &sMxMatrix::RotX(sMxAng a)
{ mx_mk_rot_x_mat(this,a.a); return *this; }
inline sMxMatrix &sMxMatrix::RotY(sMxAng a)
{ mx_mk_rot_y_mat(this,a.a); return *this; }
inline sMxMatrix &sMxMatrix::RotZ(sMxAng a)
{ mx_mk_rot_z_mat(this,a.a); return *this; }

// rotate matrix about that axis in its own frame by ang
inline sMxMatrix &sMxMatrix::RotX(const sMxMatrix &m,sMxAng a)
{ mx_rot_x_mat(this,&m,a.a); return *this; }
inline sMxMatrix &sMxMatrix::RotY(const sMxMatrix &m,sMxAng a)
{ mx_rot_y_mat(this,&m,a.a); return *this; }
inline sMxMatrix &sMxMatrix::RotZ(const sMxMatrix &m,sMxAng a)
{ mx_rot_z_mat(this,&m,a.a); return *this; }

// Rotate about a vector
inline sMxMatrix &sMxMatrix::RotVec(const sMxVector &v, sMxAng a)
{ mx_mk_rot_vec_mat(this,&v,a.a); return *this;}

#endif
