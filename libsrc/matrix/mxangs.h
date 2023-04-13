/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/matrix/rcs/mxangs.h 1.6 1998/10/29 11:31:00 jaemz Exp $
// angle/angvec stuff for mx
// since it is single/double precision invariant

#ifndef __MXANGS_H
#define __MXANGS_H

// for fmod
#include <math.h>

#include <mxang.h>
#include <mxmats.h>


struct sMxAng
{
   mxs_ang a;

   // Null Constructor
   inline sMxAng() {}

   // Constructor from mxs_ang
   inline sMxAng(mxs_ang _a) {a = _a;}

   // Sets from radians and degrees
   inline sMxAng &Set(mxs_ang _a) {a = _a; return *this;}

   inline sMxAng &SetRad(float rad) 
   {a = rad*(MX_ANG_PI/MX_REAL_PI); return *this;}
   inline sMxAng &SetRad(double rad) 
   {a = rad*(MX_ANG_PI/MX_REAL_PI); return *this;}

   inline sMxAng &SetDeg(float rad) 
   {a = rad*(MX_ANG_PI/180.0); return *this;}
   inline sMxAng &SetDeg(double rad) 
   {a = rad*(MX_ANG_PI/180.0); return *this;}
   
   // Gets radians and degrees
   const inline float Rad() {return (float)a * (MX_REAL_PI/MX_ANG_PI);}
   const inline float Deg() {return (float)a * (180.0/MX_ANG_PI);}
   
   // Signed version
   const inline float RadSigned() {return (float)((short)a) * (MX_REAL_PI/MX_ANG_PI);}
   const inline float DegSigned() {return (float)((short)a) * (180.0/MX_ANG_PI);}

   const inline mxs_ang Ang() {return a;}

   // Sins, cosines, etc
   inline float Sin() {return mx_sin(this->Ang());}
   inline float Cos() {return mx_cos(this->Ang());}
   inline void SinCos(float *s,float *c) {mx_sincos(this->Ang(),s,c);}
   inline void SinCos(double *s,double *c) {
      float fs,fc;
      mx_sincos(this->Ang(),&fs,&fc);
      *s = fs;
      *c = fc;
   }
};

struct sMxAngVec : mxs_angvec
{
   // constructors
   // null
   inline sMxAngVec() {}

   // mxs_angvec
   inline sMxAngVec(mxs_angvec _a) {tx=_a.tx;ty=_a.ty;tz=_a.tz;}

   // 3 sMxAngs
   inline sMxAngVec(sMxAng a,sMxAng b,sMxAng c) {tx=a.Ang();ty=b.Ang();tz=c.Ang();}

   // sMxRadVec
   inline sMxAngVec(sMxRadVec _r);

   // sMxMatrix (do the standard decomposition)
   inline sMxAngVec(sMxMatrix _m) 
   {mx_mat2ang(this,&_m);}

   // Accessors to let you get at the angles as sMxAngs
   inline sMxAng &Tx() {return *(sMxAng *)(&tx);}
   inline sMxAng &Ty() {return *(sMxAng *)(&ty);}
   inline sMxAng &Tz() {return *(sMxAng *)(&tz);}

   // Setters for the same
   inline sMxAngVec &Set(sMxAng a,sMxAng b,sMxAng c) 
   {tx=a.Ang();ty=b.Ang();tz=c.Ang();return *this;}

   inline sMxAngVec &Set(sMxRadVec _r);

   // Sets from a vector of degrees
   inline sMxAngVec &SetDeg(const sMxVector &_r) {
      tx=_r.x*(MX_ANG_PI/180.0);
      ty=_r.y*(MX_ANG_PI/180.0);
      tz=_r.z*(MX_ANG_PI/180.0); return *this;}

   // Stuffs into a vector of degrees
   // Treats as unsigned so beware
   inline void StuffDeg(sMxVector *_r) {
      _r->x = (float)tx*(180.0/MX_ANG_PI);
      _r->y = (float)ty*(180.0/MX_ANG_PI);
      _r->z = (float)tz*(180.0/MX_ANG_PI);
   }

   // sMxMatrix (do the standard decomposition)
   inline sMxAngVec &Set(sMxMatrix _m) 
   {mx_mat2ang(this,&_m); return *this;}

   inline sMxAngVec &Zero() {tx=0;ty=0;tz=0;return *this;}

   // operators

   inline sMxAngVec &operator=(const sMxAngVec &a)
   {tx=a.tx;ty=a.ty;tz=a.tz;return *this;}

   inline sMxAngVec operator+(const sMxAngVec &v) const
   { return sMxAngVec(tx+v.tx,ty+v.ty,tz+v.tz); }

   inline sMxAngVec &operator+=(const sMxAngVec &v)
   { tx+=v.tx; ty+=v.ty; tz+=v.tz; return *this; } 

   // Subtraction
   inline sMxAngVec operator-() const
   { return sMxAngVec(-tx,-ty,-tz); }   
   inline sMxAngVec operator-(const sMxAngVec &v) const
   { return sMxAngVec(tx-v.tx,ty-v.ty,tz-v.tz); }
   inline sMxAngVec &operator-=(const sMxAngVec &v)
   { tx-=v.tx; ty-=v.ty; tz-=v.tz; return *this; } 
   inline sMxAngVec &NegEq()
   { tx=-tx; ty=-ty; tz=-tz; return *this; }
   
   // Scalar Multiply
   inline sMxAngVec operator*(float s) const
   { return sMxAngVec(tx*s,ty*s,tz*s); } 
   sMxAngVec &operator*=(float s)
   { tx*=s; ty*=s; tz*=s; return *this; }

   // Scalar Divide
   sMxAngVec operator/(float s) const
   { return sMxAngVec(tx/s,ty/s,tz/s); } 
   sMxAngVec &operator/=(float s)
   { tx/=s; ty/=s; tz/=s; return *this; }
};


// this is so that the Angvec isn't totally alone in the world
// This is for radians, and is inherited from the vector and gets
// all its methods.  The idea being that we should have an actual
// type for this rather than not, so that implicit conversions through
// the equals operators aren't obscure
struct sMxRadVec : sMxVector
{
   inline sMxRadVec() {}
   inline sMxRadVec(float _x,float _y,float _z) {x=_x;y=_y;z=_z;}
   inline sMxRadVec(const sMxRadVec &v) 
   {x=v.x;y=v.y;z=v.z;}
   
   // AngVec
   inline sMxRadVec(const sMxAngVec &a) { 
      x=(float)a.tx*(MX_REAL_PI/MX_ANG_PI);
      y=(float)a.ty*(MX_REAL_PI/MX_ANG_PI);
      z=(float)a.tz*(MX_REAL_PI/MX_ANG_PI); }

   // Matrix
   inline sMxRadVec(const sMxMatrix &m) 
   { mx_mat2rad(this,&m);}

   // Setters for these two
   inline sMxRadVec &Set(float _x,float _y,float _z) 
   {x=_x;y=_y;z=_z; return *this;}

   inline sMxRadVec &Set(const sMxAngVec &a) { 
      x=(float)a.tx*(MX_REAL_PI/MX_ANG_PI);
      y=(float)a.ty*(MX_REAL_PI/MX_ANG_PI);
      z=(float)a.tz*(MX_REAL_PI/MX_ANG_PI); return *this;}

   inline sMxRadVec &Set(const sMxMatrix &m) 
   { mx_mat2rad(this,&m); return *this;}

   // Renormalizer
   // Forces all between -2pi and pi
   inline sMxRadVec &Renorm()
   {  x = fmod(x,(float)MX_REAL_2PI);
      y = fmod(y,(float)MX_REAL_2PI);
      z = fmod(z,(float)MX_REAL_2PI); return *this; }
};


// sMxAngVec/sMxRadVec
inline sMxAngVec::sMxAngVec(sMxRadVec _r) 
{  tx=_r.x*(MX_ANG_PI/MX_REAL_PI);
   ty=_r.y*(MX_ANG_PI/MX_REAL_PI);
   tz=_r.z*(MX_ANG_PI/MX_REAL_PI);}


inline sMxAngVec &sMxAngVec::Set(sMxRadVec _r) 
{  tx=_r.x*(MX_ANG_PI/MX_REAL_PI);
   ty=_r.y*(MX_ANG_PI/MX_REAL_PI);
   tz=_r.z*(MX_ANG_PI/MX_REAL_PI); return *this;}

#endif  // __MXANGS_H










