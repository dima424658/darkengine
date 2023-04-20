/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/matrix/RCS/mxmats.h 1.11 1999/07/21 15:44:10 JAEMZ Exp $

#ifndef __MXMATS_H
#define __MXMATS_H
#pragma once

// Structure definition file for the mats, but basically has everything
// except assignment operators for the other precision variant of this
// class

#include <math.h>
#include <matrix.h>

/*
There are some conventions here.
1) Nothing returns an sMxVector.  It's too inefficient, since
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
   mxs_vector a;
   sMxVector b,c;
   a = b+c;
   
   It will work, but the return value of b+c gets copied into a temporary
   then assigned to a.  Making the + operator return an mxs_vector doesn't
   fix the problem.  So use sMxVector pervasively, when you do..        
*/

// Double precision vector forward declared
struct sMxdVector;
struct sMxMatrix;
struct sMxTrans;
struct sMxRadVec;
struct sMxAngVec;
struct sMxAng;

// Here it is folks, the proud sMxVector
struct sMxVector : mxs_vector
{
   inline sMxVector() {}
   inline sMxVector(float _x,float _y,float _z) {x=_x;y=_y;z=_z;}
   inline sMxVector(const mxs_vector &v) {x=v.x;y=v.y;z=v.z;}
   inline sMxVector(const sMxdVector &v);
   inline sMxVector &operator=(const sMxdVector &v);
   inline sMxVector &operator=(const sMxVector &v) 
   {x=v.x;y=v.y;z=v.z;return *this;}

   // Zers
   inline sMxVector &Zero() {x=0;y=0;z=0; return *this;}

   // Setting operator
   inline sMxVector &Set(float _x,float _y,float _z) 
   {x=_x;y=_y;z=_z; return *this;}

   // Addition operators
   inline sMxVector operator+(const sMxVector &v) const
   { return sMxVector(x+v.x,y+v.y,z+v.z); }

   inline sMxVector &operator+=(const sMxVector &v)
   { x+=v.x; y+=v.y; z+=v.z; return *this; } 

   // Subtraction
   inline sMxVector operator-() const
   { return sMxVector(-x,-y,-z); }   
   inline sMxVector operator-(const sMxVector &v) const 
   { return sMxVector(x-v.x,y-v.y,z-v.z); }
   inline sMxVector &operator-=(const sMxVector &v)
   { x-=v.x; y-=v.y; z-=v.z; return *this; } 
   inline sMxVector &NegEq()
   { x=-x; y=-y; z=-z; return *this; }
   
   // Element wise multiply
   inline sMxVector operator*(const sMxVector &v) const
   { return sMxVector(x*v.x,y*v.y,z*v.z); }
   inline sMxVector &operator*=(const sMxVector &v)
   { x*=v.x; y*=v.y; z*=v.z; return *this; } 

   // Element wise divide
   inline sMxVector operator/(const sMxVector &v) const
   { return sMxVector(x/v.x,y/v.y,z/v.z); }
   inline sMxVector &operator/=(const sMxVector &v)
   { x/=v.x; y/=v.y; z/=v.z; return *this; }

   // Scalar Multiply
   inline sMxVector operator*(float s) const
   { return sMxVector(x*s,y*s,z*s); } 
   sMxVector &operator*=(float s)
   { x*=s; y*=s; z*=s; return *this; }

   // Scalar Divide
   sMxVector operator/(float s) const
   { return sMxVector(x/s,y/s,z/s); } 
   sMxVector &operator/=(float s)
   { x/=s; y/=s; z/=s; return *this; }

   // Returns this = v1 + v2*s
   sMxVector &ScaleAdd(const sMxVector &v1,const sMxVector &v2,float s)
   { mx_scale_add_vec(this,&v1,&v2,s); return *this; }

   // Returns this = this + v*s;
   sMxVector &ScaleAddEq(const sMxVector &v,float s)
   { mx_scale_addeq_vec(this,&v,s); return *this; }

   // Interpolates between this and v fraction s
   sMxVector &Interpolate(const sMxVector &v1,const sMxVector &v2,float s)
   { mx_interpolate_vec(this,&v1,&v2,s); return *this; }
   
   // Distance squared from this to v
   float Dist2(const sMxVector &v) const
   { return mx_dist2_vec(this,&v); }

   // Distance from this to v
   float Dist(const sMxVector &v) const
   { return mx_dist_vec(this,&v); }

   // Magnitude squared and unsquared
   inline float Mag2() const { return x*x+y*y+z*z; } 

   float Mag() const { return mx_mag_vec(this); }

   // makes this Normal of v, returns mag of v
   float Norm(const sMxVector &v)
   { return mx_norm_vec(this,&v); }

   // Normalizes self
   float NormEq()
   { return mx_normeq_vec(this); }

   // Dot product of this and v
   inline float Dot(const sMxVector &v) const { return x*v.x+y*v.y+z*v.z; }

   // Cross product of v1 and v2.
   sMxVector &Cross(const sMxVector &v1,const sMxVector &v2)
   { mx_cross_vec(this,&v1,&v2); return *this; }

   // Normalized version returns norm(this x v)
   float CrossNorm(const sMxVector &v1,const sMxVector &v2)
   { return mx_cross_norm_vec(this,&v1,&v2); }

   // Returns elementwise min of this and v
   sMxVector &Min(const sMxVector &v1,const sMxVector &v2)
   { mx_min_vec(this,&v1,&v2); return *this; }

   sMxVector &MinEq(const sMxVector &v)
   { mx_mineq_vec(this,&v); return *this; }

   sMxVector &Max(const sMxVector &v1,const sMxVector &v2)
   { mx_max_vec(this,&v1,&v2); return *this; }
   sMxVector &MaxEq(const sMxVector &v)
   { mx_maxeq_vec(this,&v); return *this;}

   sMxVector &Abs(const sMxVector &v)
   { x=fabs(v.x);y=fabs(v.y);z=fabs(v.z); return *this;}
   sMxVector &AbsEq()
   { x=fabs(x);y=fabs(y);z=fabs(z); return *this;}

   // Returns TRUE if all element deltas within eps
   bool IsIdentical(const sMxVector &v,float eps)
   { return mx_is_identical(this,&v,eps); }

   // Vector Matrix functions:
   // dest = M x v
   inline sMxVector &Mul(const sMxMatrix &m,const sMxVector &v);
   // v x= M, this is idiotic, if anyone uses it, I'll kill them        
   inline sMxVector &MulEq(const sMxMatrix &m);
   // dest = M^t x v, this is for multing by inverse if unit
   inline sMxVector &TransMul(const sMxMatrix &m,const sMxVector &v);
   // v x= M^t
   inline sMxVector &TransMulEq(const sMxMatrix &m);

   // Vector Transform functions:
   // multiply tform by vector
   // this = t * v
   sMxVector &Mul(const sMxTrans &t,const sMxVector &v);
   // this = T^t * v
   sMxVector &TransMul(const sMxTrans &t,const sMxVector &v);

   // Vector angle ops, rotate vector around the axis by a
   inline sMxVector &RotX(const sMxVector &v,const sMxAng &a);
   inline sMxVector &RotY(const sMxVector &v,const sMxAng &a);
   inline sMxVector &RotZ(const sMxVector &v,const sMxAng &a);
};




// Operators not operating on themselves (like * compared to *=) are
// intentionally not provided since they would have to copy.  Instead
// Scale and Div must be explicitly called

// Here it is folks, the proud sMxVector
struct sMxMatrix : mxs_matrix
{
   inline sMxMatrix() {}
   inline sMxMatrix(float m1,float m2,float m3,
                    float m4,float m5,float m6,
                    float m7,float m8,float m9) 
   {mx_mk_mat(this,m1,m2,m3,m4,m5,m6,m7,m8,m9);}

   inline sMxMatrix(const mxs_matrix &m)
   {mx_mk_mat(this,m.m0,m.m1,m.m2,m.m3,m.m4,m.m5,m.m6,m.m7,m.m8);}

   inline sMxMatrix(const mxs_vector &v1,
                    const mxs_vector &v2,
                    const mxs_vector &v3) 
   {vec[0]=v1;vec[1]=v2;vec[2]=v3;}

   inline sMxMatrix(const sMxAngVec &a);
   
   inline sMxMatrix(const sMxRadVec &r);

   // Accessors to members as vectors
   inline sMxVector &Vec(int i) {return *(sMxVector *)&(this->vec[i]);}
   
   // Setting operator
   // From 9 floats
   inline sMxMatrix &Set(float m1,float m2,float m3,
                         float m4,float m5,float m6,
                         float m7,float m8,float m9) 
   {mx_mk_mat(this,m1,m2,m3,m4,m5,m6,m7,m8,m9); return *this;}

   // From 3 vectors
   inline sMxMatrix &Set(const mxs_vector &v1,
                         const mxs_vector &v2,
                         const mxs_vector &v3) 
   {vec[0]=v1;vec[1]=v2;vec[2]=v3; return *this;}

   inline sMxMatrix &Set(const sMxAngVec &a);
   inline sMxMatrix &Set(const sMxRadVec &a);
   
   inline sMxMatrix &Zero() {mx_zero_mat(this); return *this;}
   inline sMxMatrix &Identity() {mx_identity_mat(this);return *this;}

   // Scaling by a scalar
   inline sMxMatrix &Scale(const sMxMatrix &m,float s) 
   { mx_scale_mat(this,&m,s); return *this; } 
   inline sMxMatrix &ScaleEq(float s) 
   {mx_scaleeq_mat(this,s);return *this;}
   inline sMxMatrix &operator *=(float s)
   {mx_scaleeq_mat(this,s);return *this;}

   // Dividing by a scalar
   inline sMxMatrix &Divide(const sMxMatrix &m,float s) 
   { mx_div_mat(this,&m,s); return *this; } 
   inline sMxMatrix &DivideEq(float s) 
   {mx_diveq_mat(this,s);return *this;}
   inline sMxMatrix &operator /=(float s)
   {mx_diveq_mat(this,s);return *this;}

   // Determinant
   inline float Determinant() 
   {return mx_det_mat(this);}

   // Transpose
   inline sMxMatrix &Transpose(const sMxMatrix &m) 
   {mx_trans_mat(this,&m);return *this;}
   inline sMxMatrix &TransposeEq() 
   {mx_transeq_mat(this);return *this;}

   // Inverse, returns false if matrix degeneratee
   inline bool Inverse(const sMxMatrix &m) 
   {return mx_inv_mat(this,&m);}
   inline bool InverseEq() 
   {return mx_inveq_mat(this);}

   // Normalize each column
   inline sMxMatrix &NormalizeCol(const sMxMatrix &m) 
   {mx_normcol_mat(this,&m);return *this;}
   inline sMxMatrix &NormalizeColEq() 
   {mx_normcoleq_mat(this);return *this;}

   // Normalize each row
   inline sMxMatrix &NormalizeRow(const sMxMatrix &m) 
   {mx_normrow_mat(this,&m);return *this;}
   inline sMxMatrix &NormalizeRowEq() 
   {mx_normroweq_mat(this);return *this;}

   // Sanitize the matrix to make determinant 1 no matter what
   inline sMxMatrix &Sanitize(const sMxMatrix &m,float tol) 
   {mx_sanitize_mat(this,&m,tol);return *this;}
   inline sMxMatrix &SanitizeEq(float tol) 
   {mx_sanitizeeq_mat(this,tol);return *this;}

   // Multiply a matrix by a matrix
   inline sMxMatrix &Mul(const sMxMatrix &m1,const sMxMatrix &m2)
   {mx_mul_mat(this,&m1,&m2);return *this;}
   inline sMxMatrix &MulEq(const sMxMatrix &m) 
   {mx_muleq_mat(this,&m);return *this;}
   
   // Multiply a matrix transpose by a matrix
   // this = m1^t x m2
   inline sMxMatrix &TransMul(const sMxMatrix &m1,const sMxMatrix &m2)
   {mx_tmul_mat(this,&m1,&m2);return *this;}

   // Multiply a matrix by a matrix transpose
   // this = m1 x m2^t
   inline sMxMatrix &MulTrans(const sMxMatrix &m1,const sMxMatrix &m2)
   {mx_mult_mat(this,&m1,&m2);return *this;}

   inline sMxMatrix &ElMul(const sMxMatrix &m1,const sMxMatrix &m2)
   {mx_elmul_mat(this,&m1,&m2);return *this;}

   inline sMxMatrix &ElMulEq(const sMxMatrix &m)
   {mx_elmuleq_mat(this,&m);return *this;}

   // Swap 2 rows, indexed from 0
   inline sMxMatrix &SwapRow(const sMxMatrix &m,int n1,int n2)
   {mx_swaprow_mat(this,&m,n1,n2);return *this;}
   inline sMxMatrix &SwapRowEq(int n1,int n2)
   {mx_swaproweq_mat(this,n1,n2);return *this;}

   // Swap 2 columns, indexed from 0
   inline sMxMatrix &SwapCol(const sMxMatrix &m,int n1,int n2)
   {mx_swapcol_mat(this,&m,n1,n2);return *this;}
   inline sMxMatrix &SwapColEq(int n1,int n2)
   {mx_swapcoleq_mat(this,n1,n2);return *this;}

   // Matrix Vector Ops
   // this = M .* v
   inline sMxMatrix &ElMul(const sMxMatrix &m,const sMxVector &v)
   {mx_mat_elmul_vec(this,&m,&v); return *this;}
   inline sMxMatrix &ElMulEq(const sMxVector &v)
   {mx_mat_elmuleq_vec(this,&v); return *this;}

   // this = v^t .* M
   inline sMxMatrix &ElTransMul(const sMxMatrix &m,const sMxVector &v)
   {mx_mat_eltmul_vec(this,&m,&v); return *this;}
   inline sMxMatrix &ElTransMulEq(const sMxVector &v)
   {mx_mat_eltmuleq_vec(this,&v); return *this;}

   // Matrix angle ops
   // Creates a matrix rotated about that axis by radian amount
   inline sMxMatrix &RotX(sMxAng a);
   inline sMxMatrix &RotY(sMxAng a);
   inline sMxMatrix &RotZ(sMxAng a);

   // rotate matrix about that axis in its own frame by ang
   inline sMxMatrix &RotX(const sMxMatrix &m,sMxAng a);
   inline sMxMatrix &RotY(const sMxMatrix &m,sMxAng a);
   inline sMxMatrix &RotZ(const sMxMatrix &m,sMxAng a);

   // Rotate about a vector
   inline sMxMatrix &RotVec(const sMxVector &v, sMxAng a);

   // Create a matrix rotated about that axis by radian amount
   inline sMxMatrix &RotXRad(float rad)
   { mx_mk_rot_x_mat_rad(this,rad); return *this;}
   inline sMxMatrix &RotYRad(float rad)
   { mx_mk_rot_y_mat_rad(this,rad); return *this;}
   inline sMxMatrix &RotZRad(float rad)
   { mx_mk_rot_z_mat_rad(this,rad); return *this;}

   // rotate matrix about that axis in its own frame by ang
   inline sMxMatrix &RotXRad(const sMxMatrix &m,float rad)
   { mx_rot_x_mat_rad(this,&m,rad); return *this; }
   inline sMxMatrix &RotYRad(const sMxMatrix &m,float rad)
   { mx_rot_y_mat_rad(this,&m,rad); return *this; }
   inline sMxMatrix &RotZRad(const sMxMatrix &m,float rad)
   { mx_rot_z_mat_rad(this,&m,rad); return *this; }

   // Utility
   // Create a matrix from an unormalized vector v, which points
   // in the direction of something (x vector) and takes a z vector.
   // returns |v|
   // Safe looks at x and y components first and uses an x vector
   // if its looking straight down
   inline float LookAtZ(const sMxVector &v,const sMxVector &z)
   { return mx_mat_look_at_z(this,&v,&z); }
   inline float LookAt(const sMxVector &v)
   { return mx_mat_look_at(this,&v); }
   inline float LookAtSafe(const sMxVector &v)
   { return mx_mat_look_at_safe(this,&v); }
};

struct sMxTrans : mxs_trans
{
   // Construct from Null
   inline sMxTrans() {}
   // 12 floats!
   inline sMxTrans(float m1,float m2,float m3,
                    float m4,float m5,float m6,
                    float m7,float m8,float m9,
                    float v1,float v2,float v3)
   {mx_mk_trans(this,m1,m2,m3,m4,m5,m6,m7,m8,m9,v1,v2,v3);}

   // A vector and a matrix
   inline sMxTrans(const mxs_vector &v,const mxs_matrix &m)
   {vec=v;mat=m;}

   // Accessors to elements as the C++ classes
   inline sMxVector &Vec() {return *(sMxVector *)&vec;}
   inline sMxMatrix &Mat() {return *(sMxMatrix *)&mat;}

   // Set from 12 floats
   inline sMxTrans &Set(float m1,float m2,float m3,
                    float m4,float m5,float m6,
                    float m7,float m8,float m9,
                    float v1,float v2,float v3)
   {mx_mk_trans(this,m1,m2,m3,m4,m5,m6,m7,m8,m9,v1,v2,v3); return *this;}

   // A matrix and a vector
   inline sMxTrans &Set(const mxs_vector &v,const mxs_matrix &m)
   {vec=v;mat=m; return *this;}

   // identity transform
   inline sMxTrans &Identity()
   {mx_identity_trans(this); return *this;}

   // invert transform that is unitary, dest != src
   inline sMxTrans &Transpose(const sMxTrans &t)
   {mx_transpose_trans(this,&t); return *this;}

   // multiply 2 transforms
   // this = t1 * t2
   inline sMxTrans &Mul(const sMxTrans &t1,const sMxTrans &t2)
   {mx_mul_trans(this,&t1,&t2); return *this;}

   // transpose pre multiply 2 transforms
   // for instance, putting view onto a stack
   // this = t1^t * t2
   inline sMxTrans &TransMul(const sMxTrans &t1,const sMxTrans &t2)
   {mx_tmul_trans(this,&t1,&t2); return *this;}

   // transpose post multiply 2 transforms
   // for instance, putting view onto a stack
   // this = t1 * t2^2
   inline sMxTrans &MulTrans(const sMxTrans &t1,const sMxTrans &t2)
   {mx_mult_trans(this,&t1,&t2); return *this;}
};





#endif //__MXMATS_H
