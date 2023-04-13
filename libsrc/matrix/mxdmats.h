/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/matrix/RCS/mxmats.h 1.11 1999/07/21 15:44:10 JAEMZ Exp $

#ifndef __MXDMATS_H
#define __MXDMATS_H
#pragma once

// Structure definition file for the mats, but basically has everything
// except assignment operators for the other precision variant of this
// class

#include <math.h>
#include <matrixd.h>

/*
There are some conventions here.
1) Nothing returns an sMxdVector.  It's too inefficient, since
   the compiler needs to create a temporary, then copy it out.
   Even a cross product is 65% slower when passing back a temporary.
   Instead, *this* is always the destination, and a ref to that is passed
   back.
2) The only case this isn't true is where operators are defined.  The
   operators are all inlined, so this isn't a problem, and temporaries
   are compiled out (at least for good compilers they are)
3) Everything takes refs, not pointers, since the intent is for all the
   methods to be operators, basically.
4) Methods with the suffix Eq use themselves as the first argument.
   Almost every method has two methods, one with Eq and one without.
   The intent is that these mimic *= vs *, that sort of thing.
5) Inlines are used very sparingly.  Only operations that should take
   nine or fewer operations are inlined.  Otherwise, they aren't, since
   there's probably no point.  In this case, only operators and constructors.

   Avoid things like:
   mxds_vector a;
   sMxdVector b,c;
   a = b+c;
   
   It will work, but the return value of b+c gets copied into a temporary
   then assigned to a.  Making the + operator return an mxds_vector doesn't
   fix the problem.  So use sMxdVector pervasively, when you do..        
*/

// Double precision vector forward declared
struct sMxVector;
struct sMxdMatrix;
struct sMxdTrans;
struct sMxRadVec;
struct sMxAngVec;
struct sMxAng;

// Here it is folks, the proud sMxdVector
struct sMxdVector : mxds_vector
{
   inline sMxdVector() {}
   inline sMxdVector(double _x,double _y,double _z) {x=_x;y=_y;z=_z;}
   inline sMxdVector(const mxds_vector &v) {x=v.x;y=v.y;z=v.z;}
   inline sMxdVector(const sMxVector &v);
   inline sMxdVector &operator=(const sMxVector &v);
   inline sMxdVector &operator=(const sMxdVector &v) 
   {x=v.x;y=v.y;z=v.z;return *this;}

   // Zers
   inline sMxdVector &Zero() {x=0;y=0;z=0; return *this;}

   // Setting operator
   inline sMxdVector &Set(double _x,double _y,double _z) 
   {x=_x;y=_y;z=_z; return *this;}

   // Addition operators
   inline sMxdVector operator+(const sMxdVector &v) const
   { return sMxdVector(x+v.x,y+v.y,z+v.z); }

   inline sMxdVector &operator+=(const sMxdVector &v)
   { x+=v.x; y+=v.y; z+=v.z; return *this; } 

   // Subtraction
   inline sMxdVector operator-() const
   { return sMxdVector(-x,-y,-z); }   
   inline sMxdVector operator-(const sMxdVector &v) const 
   { return sMxdVector(x-v.x,y-v.y,z-v.z); }
   inline sMxdVector &operator-=(const sMxdVector &v)
   { x-=v.x; y-=v.y; z-=v.z; return *this; } 
   inline sMxdVector &NegEq()
   { x=-x; y=-y; z=-z; return *this; }
   
   // Element wise multiply
   inline sMxdVector operator*(const sMxdVector &v) const
   { return sMxdVector(x*v.x,y*v.y,z*v.z); }
   inline sMxdVector &operator*=(const sMxdVector &v)
   { x*=v.x; y*=v.y; z*=v.z; return *this; } 

   // Element wise divide
   inline sMxdVector operator/(const sMxdVector &v) const
   { return sMxdVector(x/v.x,y/v.y,z/v.z); }
   inline sMxdVector &operator/=(const sMxdVector &v)
   { x/=v.x; y/=v.y; z/=v.z; return *this; }

   // Scalar Multiply
   inline sMxdVector operator*(double s) const
   { return sMxdVector(x*s,y*s,z*s); } 
   sMxdVector &operator*=(double s)
   { x*=s; y*=s; z*=s; return *this; }

   // Scalar Divide
   sMxdVector operator/(double s) const
   { return sMxdVector(x/s,y/s,z/s); } 
   sMxdVector &operator/=(double s)
   { x/=s; y/=s; z/=s; return *this; }

   // Returns this = v1 + v2*s
   sMxdVector &ScaleAdd(const sMxdVector &v1,const sMxdVector &v2,double s)
   { mxd_scale_add_vec(this,&v1,&v2,s); return *this; }

   // Returns this = this + v*s;
   sMxdVector &ScaleAddEq(const sMxdVector &v,double s)
   { mxd_scale_addeq_vec(this,&v,s); return *this; }

   // Interpolates between this and v fraction s
   sMxdVector &Interpolate(const sMxdVector &v1,const sMxdVector &v2,double s)
   { mxd_interpolate_vec(this,&v1,&v2,s); return *this; }
   
   // Distance squared from this to v
   double Dist2(const sMxdVector &v) const
   { return mxd_dist2_vec(this,&v); }

   // Distance from this to v
   double Dist(const sMxdVector &v) const
   { return mxd_dist_vec(this,&v); }

   // Magnitude squared and unsquared
   inline double Mag2() const { return x*x+y*y+z*z; } 

   double Mag() const { return mxd_mag_vec(this); }

   // makes this Normal of v, returns mag of v
   double Norm(const sMxdVector &v)
   { return mxd_norm_vec(this,&v); }

   // Normalizes self
   double NormEq()
   { return mxd_normeq_vec(this); }

   // Dot product of this and v
   inline double Dot(const sMxdVector &v) const { return x*v.x+y*v.y+z*v.z; }

   // Cross product of v1 and v2.
   sMxdVector &Cross(const sMxdVector &v1,const sMxdVector &v2)
   { mxd_cross_vec(this,&v1,&v2); return *this; }

   // Normalized version returns norm(this x v)
   double CrossNorm(const sMxdVector &v1,const sMxdVector &v2)
   { return mxd_cross_norm_vec(this,&v1,&v2); }

   // Returns elementwise min of this and v
   sMxdVector &Min(const sMxdVector &v1,const sMxdVector &v2)
   { mxd_min_vec(this,&v1,&v2); return *this; }

   sMxdVector &MinEq(const sMxdVector &v)
   { mxd_mineq_vec(this,&v); return *this; }

   sMxdVector &Max(const sMxdVector &v1,const sMxdVector &v2)
   { mxd_max_vec(this,&v1,&v2); return *this; }
   sMxdVector &MaxEq(const sMxdVector &v)
   { mxd_maxeq_vec(this,&v); return *this;}

   sMxdVector &Abs(const sMxdVector &v)
   { x=fabs(v.x);y=fabs(v.y);z=fabs(v.z); return *this;}
   sMxdVector &AbsEq()
   { x=fabs(x);y=fabs(y);z=fabs(z); return *this;}

   // Returns TRUE if all element deltas within eps
   bool IsIdentical(const sMxdVector &v,double eps)
   { return mxd_is_identical(this,&v,eps); }

   // Vector Matrix functions:
   // dest = M x v
   inline sMxdVector &Mul(const sMxdMatrix &m,const sMxdVector &v);
   // v x= M, this is idiotic, if anyone uses it, I'll kill them        
   inline sMxdVector &MulEq(const sMxdMatrix &m);
   // dest = M^t x v, this is for multing by inverse if unit
   inline sMxdVector &TransMul(const sMxdMatrix &m,const sMxdVector &v);
   // v x= M^t
   inline sMxdVector &TransMulEq(const sMxdMatrix &m);

   // Vector Transform functions:
   // multiply tform by vector
   // this = t * v
   sMxdVector &Mul(const sMxdTrans &t,const sMxdVector &v);
   // this = T^t * v
   sMxdVector &TransMul(const sMxdTrans &t,const sMxdVector &v);

   // Vector angle ops, rotate vector around the axis by a
   inline sMxdVector &RotX(const sMxdVector &v,const sMxAng &a);
   inline sMxdVector &RotY(const sMxdVector &v,const sMxAng &a);
   inline sMxdVector &RotZ(const sMxdVector &v,const sMxAng &a);
};




// Operators not operating on themselves (like * compared to *=) are
// intentionally not provided since they would have to copy.  Instead
// Scale and Div must be explicitly called

// Here it is folks, the proud sMxdVector
struct sMxdMatrix : mxds_matrix
{
   inline sMxdMatrix() {}
   inline sMxdMatrix(double m1,double m2,double m3,
                    double m4,double m5,double m6,
                    double m7,double m8,double m9) 
   {mxd_mk_mat(this,m1,m2,m3,m4,m5,m6,m7,m8,m9);}

   inline sMxdMatrix(const mxds_matrix &m)
   {mxd_mk_mat(this,m.m0,m.m1,m.m2,m.m3,m.m4,m.m5,m.m6,m.m7,m.m8);}

   inline sMxdMatrix(const mxds_vector &v1,
                    const mxds_vector &v2,
                    const mxds_vector &v3) 
   {vec[0]=v1;vec[1]=v2;vec[2]=v3;}

   inline sMxdMatrix(const sMxAngVec &a);
   
   inline sMxdMatrix(const sMxRadVec &r);

   // Accessors to members as vectors
   inline sMxdVector &Vec(int i) {return *(sMxdVector *)&(this->vec[i]);}
   
   // Setting operator
   // From 9 doubles
   inline sMxdMatrix &Set(double m1,double m2,double m3,
                         double m4,double m5,double m6,
                         double m7,double m8,double m9) 
   {mxd_mk_mat(this,m1,m2,m3,m4,m5,m6,m7,m8,m9); return *this;}

   // From 3 vectors
   inline sMxdMatrix &Set(const mxds_vector &v1,
                         const mxds_vector &v2,
                         const mxds_vector &v3) 
   {vec[0]=v1;vec[1]=v2;vec[2]=v3; return *this;}

   inline sMxdMatrix &Set(const sMxAngVec &a);
   inline sMxdMatrix &Set(const sMxRadVec &a);
   
   inline sMxdMatrix &Zero() {mxd_zero_mat(this); return *this;}
   inline sMxdMatrix &Identity() {mxd_identity_mat(this);return *this;}

   // Scaling by a scalar
   inline sMxdMatrix &Scale(const sMxdMatrix &m,double s) 
   { mxd_scale_mat(this,&m,s); return *this; } 
   inline sMxdMatrix &ScaleEq(double s) 
   {mxd_scaleeq_mat(this,s);return *this;}
   inline sMxdMatrix &operator *=(double s)
   {mxd_scaleeq_mat(this,s);return *this;}

   // Dividing by a scalar
   inline sMxdMatrix &Divide(const sMxdMatrix &m,double s) 
   { mxd_div_mat(this,&m,s); return *this; } 
   inline sMxdMatrix &DivideEq(double s) 
   {mxd_diveq_mat(this,s);return *this;}
   inline sMxdMatrix &operator /=(double s)
   {mxd_diveq_mat(this,s);return *this;}

   // Determinant
   inline double Determinant() 
   {return mxd_det_mat(this);}

   // Transpose
   inline sMxdMatrix &Transpose(const sMxdMatrix &m) 
   {mxd_trans_mat(this,&m);return *this;}
   inline sMxdMatrix &TransposeEq() 
   {mxd_transeq_mat(this);return *this;}

   // Inverse, returns false if matrix degeneratee
   inline bool Inverse(const sMxdMatrix &m) 
   {return mxd_inv_mat(this,&m);}
   inline bool InverseEq() 
   {return mxd_inveq_mat(this);}

   // Normalize each column
   inline sMxdMatrix &NormalizeCol(const sMxdMatrix &m) 
   {mxd_normcol_mat(this,&m);return *this;}
   inline sMxdMatrix &NormalizeColEq() 
   {mxd_normcoleq_mat(this);return *this;}

   // Normalize each row
   inline sMxdMatrix &NormalizeRow(const sMxdMatrix &m) 
   {mxd_normrow_mat(this,&m);return *this;}
   inline sMxdMatrix &NormalizeRowEq() 
   {mxd_normroweq_mat(this);return *this;}

   // Sanitize the matrix to make determinant 1 no matter what
   inline sMxdMatrix &Sanitize(const sMxdMatrix &m,double tol) 
   {mxd_sanitize_mat(this,&m,tol);return *this;}
   inline sMxdMatrix &SanitizeEq(double tol) 
   {mxd_sanitizeeq_mat(this,tol);return *this;}

   // Multiply a matrix by a matrix
   inline sMxdMatrix &Mul(const sMxdMatrix &m1,const sMxdMatrix &m2)
   {mxd_mul_mat(this,&m1,&m2);return *this;}
   inline sMxdMatrix &MulEq(const sMxdMatrix &m) 
   {mxd_muleq_mat(this,&m);return *this;}
   
   // Multiply a matrix transpose by a matrix
   // this = m1^t x m2
   inline sMxdMatrix &TransMul(const sMxdMatrix &m1,const sMxdMatrix &m2)
   {mxd_tmul_mat(this,&m1,&m2);return *this;}

   // Multiply a matrix by a matrix transpose
   // this = m1 x m2^t
   inline sMxdMatrix &MulTrans(const sMxdMatrix &m1,const sMxdMatrix &m2)
   {mxd_mult_mat(this,&m1,&m2);return *this;}

   inline sMxdMatrix &ElMul(const sMxdMatrix &m1,const sMxdMatrix &m2)
   {mxd_elmul_mat(this,&m1,&m2);return *this;}

   inline sMxdMatrix &ElMulEq(const sMxdMatrix &m)
   {mxd_elmuleq_mat(this,&m);return *this;}

   // Swap 2 rows, indexed from 0
   inline sMxdMatrix &SwapRow(const sMxdMatrix &m,int n1,int n2)
   {mxd_swaprow_mat(this,&m,n1,n2);return *this;}
   inline sMxdMatrix &SwapRowEq(int n1,int n2)
   {mxd_swaproweq_mat(this,n1,n2);return *this;}

   // Swap 2 columns, indexed from 0
   inline sMxdMatrix &SwapCol(const sMxdMatrix &m,int n1,int n2)
   {mxd_swapcol_mat(this,&m,n1,n2);return *this;}
   inline sMxdMatrix &SwapColEq(int n1,int n2)
   {mxd_swapcoleq_mat(this,n1,n2);return *this;}

   // Matrix Vector Ops
   // this = M .* v
   inline sMxdMatrix &ElMul(const sMxdMatrix &m,const sMxdVector &v)
   {mxd_mat_elmul_vec(this,&m,&v); return *this;}
   inline sMxdMatrix &ElMulEq(const sMxdVector &v)
   {mxd_mat_elmuleq_vec(this,&v); return *this;}

   // this = v^t .* M
   inline sMxdMatrix &ElTransMul(const sMxdMatrix &m,const sMxdVector &v)
   {mxd_mat_eltmul_vec(this,&m,&v); return *this;}
   inline sMxdMatrix &ElTransMulEq(const sMxdVector &v)
   {mxd_mat_eltmuleq_vec(this,&v); return *this;}

   // Matrix angle ops
   // Creates a matrix rotated about that axis by radian amount
   inline sMxdMatrix &RotX(sMxAng a);
   inline sMxdMatrix &RotY(sMxAng a);
   inline sMxdMatrix &RotZ(sMxAng a);

   // rotate matrix about that axis in its own frame by ang
   inline sMxdMatrix &RotX(const sMxdMatrix &m,sMxAng a);
   inline sMxdMatrix &RotY(const sMxdMatrix &m,sMxAng a);
   inline sMxdMatrix &RotZ(const sMxdMatrix &m,sMxAng a);

   // Rotate about a vector
   inline sMxdMatrix &sMxdMatrix::RotVec(const sMxdVector &v, sMxAng a);

   // Create a matrix rotated about that axis by radian amount
   inline sMxdMatrix &RotXRad(double rad)
   { mxd_mk_rot_x_mat_rad(this,rad); return *this;}
   inline sMxdMatrix &RotYRad(double rad)
   { mxd_mk_rot_y_mat_rad(this,rad); return *this;}
   inline sMxdMatrix &RotZRad(double rad)
   { mxd_mk_rot_z_mat_rad(this,rad); return *this;}

   // rotate matrix about that axis in its own frame by ang
   inline sMxdMatrix &RotXRad(const sMxdMatrix &m,double rad)
   { mxd_rot_x_mat_rad(this,&m,rad); return *this; }
   inline sMxdMatrix &RotYRad(const sMxdMatrix &m,double rad)
   { mxd_rot_y_mat_rad(this,&m,rad); return *this; }
   inline sMxdMatrix &RotZRad(const sMxdMatrix &m,double rad)
   { mxd_rot_z_mat_rad(this,&m,rad); return *this; }

   // Utility
   // Create a matrix from an unormalized vector v, which points
   // in the direction of something (x vector) and takes a z vector.
   // returns |v|
   // Safe looks at x and y components first and uses an x vector
   // if its looking straight down
   inline double LookAtZ(const sMxdVector &v,const sMxdVector &z)
   { return mxd_mat_look_at_z(this,&v,&z); }
   inline double LookAt(const sMxdVector &v)
   { return mxd_mat_look_at(this,&v); }
   inline double LookAtSafe(const sMxdVector &v)
   { return mxd_mat_look_at_safe(this,&v); }
};

struct sMxdTrans : mxds_trans
{
   // Construct from Null
   inline sMxdTrans() {}
   // 12 doubles!
   inline sMxdTrans(double m1,double m2,double m3,
                    double m4,double m5,double m6,
                    double m7,double m8,double m9,
                    double v1,double v2,double v3)
   {mxd_mk_trans(this,m1,m2,m3,m4,m5,m6,m7,m8,m9,v1,v2,v3);}

   // A vector and a matrix
   inline sMxdTrans(const mxds_vector &v,const mxds_matrix &m)
   {vec=v;mat=m;}

   // Accessors to elements as the C++ classes
   inline sMxdVector &Vec() {return *(sMxdVector *)&vec;}
   inline sMxdMatrix &Mat() {return *(sMxdMatrix *)&mat;}

   // Set from 12 doubles
   inline sMxdTrans &Set(double m1,double m2,double m3,
                    double m4,double m5,double m6,
                    double m7,double m8,double m9,
                    double v1,double v2,double v3)
   {mxd_mk_trans(this,m1,m2,m3,m4,m5,m6,m7,m8,m9,v1,v2,v3); return *this;}

   // A matrix and a vector
   inline sMxdTrans &Set(const mxds_vector &v,const mxds_matrix &m)
   {vec=v;mat=m; return *this;}

   // identity transform
   inline sMxdTrans &Identity()
   {mxd_identity_trans(this); return *this;}

   // invert transform that is unitary, dest != src
   inline sMxdTrans &Transpose(const sMxdTrans &t)
   {mxd_transpose_trans(this,&t); return *this;}

   // multiply 2 transforms
   // this = t1 * t2
   inline sMxdTrans &Mul(const sMxdTrans &t1,const sMxdTrans &t2)
   {mxd_mul_trans(this,&t1,&t2); return *this;}

   // transpose pre multiply 2 transforms
   // for instance, putting view onto a stack
   // this = t1^t * t2
   inline sMxdTrans &TransMul(const sMxdTrans &t1,const sMxdTrans &t2)
   {mxd_tmul_trans(this,&t1,&t2); return *this;}

   // transpose post multiply 2 transforms
   // for instance, putting view onto a stack
   // this = t1 * t2^2
   inline sMxdTrans &MulTrans(const sMxdTrans &t1,const sMxdTrans &t2)
   {mxd_mult_trans(this,&t1,&t2); return *this;}
};





#endif //__MXDMATS_H
