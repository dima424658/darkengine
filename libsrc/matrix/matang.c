/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matang.c $
 * $Revision: 1.16 $
 * $Author: JAEMZ $
 * $Date: 1998/02/12 15:17:59 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrix.h>

#define MX_VERSION

// Does this wrap and stuff right?
mxs_ang mx_rad2ang(mxs_real rad)
{
   return rad*MX_ANG_PI/MX_REAL_PI;
}

// Always returns positive rads
mxs_real mx_ang2rad(mxs_ang ang)
{
   return (mxs_real)ang * MX_REAL_PI/MX_ANG_PI;
}

// convert to and from degs
mxs_ang mx_deg2ang(mxs_real deg)
{
   return  deg*MX_ANG_PI/180.0;
}

mxs_real mx_ang2deg(mxs_real ang)
{
   return (mxs_real)ang * 180.0/MX_ANG_PI;
}



mxs_real mx_sin(mxs_ang ang)
{
   return sin((float)ang*MX_REAL_2PI/65536.0);
}

mxs_real mx_cos(mxs_ang ang)
{
   return cos((float)ang*MX_REAL_2PI/65536.0);
}

void mx_sincos(mxs_ang ang,mxs_real *s, mxs_real *c)
{
   // well, okay this is lame
   double rang = (float)ang*MX_REAL_2PI/65536.0;

   *s = sin(rang);
   *c = cos(rang);
}

/////////////////////////////////
// octant symmetric sin/cos calls

#define do_ang_convert(ang) ((float)ang*MX_REAL_2PI/65536.0)

#define OCT_USE_SELF 1
#define OCT_FLIP_SIN 2
#define OCT_FLIP_COS 4

// NOTE: these really get instantiated twice
//  once in mx_, once in mxd_
// that is why they are static
// since they are so small, it seemed easier to have them in both places
// so that the compiler could optimize/inline them as desired

static uchar oct_coding[]=
{
   OCT_USE_SELF,
   0,
   OCT_FLIP_COS,
   OCT_USE_SELF|OCT_FLIP_COS,
   OCT_USE_SELF|OCT_FLIP_COS|OCT_FLIP_SIN,
   OCT_FLIP_COS|OCT_FLIP_SIN,
   OCT_FLIP_SIN,
   OCT_USE_SELF|OCT_FLIP_SIN
};


// changes argument in place, returns octant code
static int _octant_relativize(mxs_ang *in_ang)
{
   int oct_code=((*in_ang)>>13)&7;
   (*in_ang)&=0x1fff;
   if (oct_code&1)
      *in_ang=0x2000-(*in_ang);
   return oct_coding[oct_code];
}

mxs_real mx_sym_sin(mxs_ang ang)
{
   int oct_code=_octant_relativize(&ang);
   float val, rang=do_ang_convert(ang);

   if (oct_code&OCT_USE_SELF)
      val=sin(rang);
   else
      val=cos(rang);
   if (oct_code&OCT_FLIP_SIN)
      val=-val;
   return val;
}

mxs_real mx_sym_cos(mxs_ang ang)
{
   int oct_code=_octant_relativize(&ang);
   float val, rang=do_ang_convert(ang);

   if (oct_code&OCT_USE_SELF)
      val=cos(rang);
   else
      val=sin(rang);
   if (oct_code&OCT_FLIP_COS)
      val=-val;
   return val;
}

void mx_sym_sincos(mxs_ang ang,mxs_real *s, mxs_real *c)
{  // well, okay this is lame
   int oct_code=_octant_relativize(&ang);
   float rang=do_ang_convert(ang);

   if (oct_code&OCT_USE_SELF)
   {
      *s = sin(rang);
     	*c = cos(rang);
   }
   else
   {
      *c = sin(rang);
     	*s = cos(rang);
   }
   if (oct_code&OCT_FLIP_SIN)
      *s=-*s;
   if (oct_code&OCT_FLIP_COS)
      *c=-*c;
}

#define USE_SYMMETRIC_TRIG
#include "mxsym.h"

void mx_mk_rot_x_mat(mxs_matrix *m,mxs_ang ang)
{
   mxs_real s,c;

   mx_sincos(ang,&s,&c);

   m->m0 = 1.0;
   m->m1 = 0;
   m->m2 = 0;
   m->m3 = 0;
   m->m6 = 0;

   m->m4 = c; m->m7 = -s;
   m->m5 = s; m->m8 = c;
}

void mx_mk_rot_y_mat(mxs_matrix *m,mxs_ang ang)
{
   mxs_real s,c;

   mx_sincos(ang,&s,&c);

   m->m4 = 1.0;
   m->m1 = 0;
   m->m3 = 0;
   m->m5 = 0;
   m->m7 = 0;

   m->m6 = s; m->m0 = c;
   m->m8 = c; m->m2 = -s;
}

void mx_mk_rot_z_mat(mxs_matrix *m,mxs_ang ang)
{
   mxs_real s,c;

   mx_sincos(ang,&s,&c);

   m->m8 = 1.0;
   m->m2 = 0;
   m->m5 = 0;
   m->m6 = 0;
   m->m7 = 0;

   m->m0 = c; m->m3 = -s;
   m->m1 = s; m->m4 = c;
}

// create rotation matrix that takes x-axis to unit vector v.
// assumes v is already normalized.
void mx_mk_move_x_mat(mxs_matrix *m, const mxs_vector *v)
{
   mxs_vector e;

   mx_copy_vec(&m->vec[0],v);

   // pick a unit vector not colinear with v
   if(fabs(v->x)>0.1||fabs(v->y)>0.1)
      mx_unit_vec(&e,2);
   else
      mx_unit_vec(&e,1);

   mx_cross_vec(&m->vec[1],&e,&m->vec[0]);
   mx_normeq_vec(&m->vec[1]);

   // this will be unit vector since v0 and v1 are unit and orthogonal
   mx_cross_vec(&m->vec[2],&m->vec[0],&m->vec[1]);
}

// create rotation matrix around vector v by angle ang
void mx_mk_rot_vec_mat(mxs_matrix *m, const mxs_vector *v, mxs_ang ang)
{
   mxs_matrix n,nInv,r;
   mxs_vector axis;

   mx_norm_vec(&axis,v);
   mx_mk_move_x_mat(&n,v);
   mx_inv_mat(&nInv,&n);
   mx_mk_rot_x_mat(&r,ang);
   mx_muleq_mat(&r,&nInv);
   mx_mul_mat(m,&n,&r);
}

// rotate matrix about that axis by ang
// equivalent to post multiplying by the matrix
// made with the make routine
void mx_rot_x_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_ang ang)
{
   mxs_real s,c;

   mx_sincos(ang,&s,&c);

   dest->m0 = m->m0;
   dest->m1 = m->m1;
   dest->m2 = m->m2;

   dest->m3 = c*m->m3 + s*m->m6;
   dest->m4 = c*m->m4 + s*m->m7;
   dest->m5 = c*m->m5 + s*m->m8;

   dest->m6 =  -s*m->m3 + c*m->m6;
   dest->m7 =  -s*m->m4 + c*m->m7;
   dest->m8 =  -s*m->m5 + c*m->m8;
}

void mx_rot_y_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_ang ang)
{
   mxs_real s,c;

   mx_sincos(ang,&s,&c);

   dest->m0 = c*m->m0 - s*m->m6;
   dest->m1 = c*m->m1 - s*m->m7;
   dest->m2 = c*m->m2 - s*m->m8;

   dest->m3 = m->m3;
   dest->m4 = m->m4;
   dest->m5 = m->m5;

   dest->m6 = s*m->m0 + c*m->m6;
   dest->m7 = s*m->m1 + c*m->m7;
   dest->m8 = s*m->m2 + c*m->m8;
}

void mx_rot_z_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_ang ang)
{
   mxs_real s,c;

   mx_sincos(ang,&s,&c);

   dest->m6 = m->m6;
   dest->m7 = m->m7;
   dest->m8 = m->m8;

   dest->m0 = c*m->m0 + s*m->m3;
   dest->m1 = c*m->m1 + s*m->m4;
   dest->m2 = c*m->m2 + s*m->m5;

   dest->m3 = -s*m->m0 + c*m->m3;
   dest->m4 = -s*m->m1 + c*m->m4;
   dest->m5 = -s*m->m2 + c*m->m5;
}

// Takes an angvec and constructs an orientation matrix
// from it, in traditional order bank (x), pitch (y), heading (z)
// a.x is bank, a.y is pitch, a.z is heading
void mx_ang2mat(mxs_matrix *dest,const mxs_angvec *a)
{
   mxs_matrix m1,m2;

   mx_mk_rot_z_mat(&m1,a->tz);
   mx_rot_y_mat(&m2,&m1,a->ty);
   mx_rot_x_mat(dest,&m2,a->tx);
}


void mx_mat2ang(mxs_angvec *dest,const mxs_matrix *m)
{
   mxs_vector v;

   mx_mat2rad(&v,m);
   dest->tx = mx_rad2ang(v.x);
   dest->ty = mx_rad2ang(v.y);
   dest->tz = mx_rad2ang(v.z);

}

// Jim went and took the plunge and implemented this.
// Converts matrix to radians, stuffed into a normal vector
// Note that it doesn't do the real math.  For instance, heading
// here (z) is really plus or minus 180 degrees
void mx_mat2rad(mxs_vector* dest, const mxs_matrix* m)
{
   // yaw is around z, pitch is around y, roll is around x

   // pitches down
   // pitch (Y) can only be -PI <= Y <= PI
   // mat->m[0][2] is -sin(Y)            

   dest->y = asin(-m->m2);

   // yaw (Z) = atan2( sinZ, cosZ)                      
   // Since cosY is always positive, this is the same as
   // atan2(sinZcosY, cosZcosY )                        
   // mat->m[0][0] is cosZcosY, mat->m[0][1] is sinZcosY

   dest->z = atan2(m->m1,m->m0);

   // Similar argument goes for roll (X)
   // mat->m[1][2] is sinXcosY, mat->m[2][2] is cosXcosY

   dest->x = atan2(m->m5,m->m8);
}

void mx_mk_rot_x_mat_rad(mxs_matrix *m, float ang)
{
   float s,c;
   s=sin(ang); c=cos(ang);

   m->m0 = 1.0;
   m->m1 = 0;
   m->m2 = 0;
   m->m3 = 0;
   m->m6 = 0;

   m->m4 = c; m->m7 = -s;
   m->m5 = s; m->m8 = c;
}

void mx_mk_rot_y_mat_rad(mxs_matrix *m, float ang)
{
   float s,c;
   s=sin(ang); c=cos(ang);

   m->m4 = 1.0;
   m->m1 = 0;
   m->m3 = 0;
   m->m5 = 0;
   m->m7 = 0;

   m->m6 = s; m->m0 = c;
   m->m8 = c; m->m2 = -s;
}

void mx_mk_rot_z_mat_rad(mxs_matrix *m, float ang)
{
   float s,c;
   s=sin(ang); c=cos(ang);
   
   m->m8 = 1.0;
   m->m2 = 0;
   m->m5 = 0;
   m->m6 = 0;
   m->m7 = 0;

   m->m0 = c; m->m3 = -s;
   m->m1 = s; m->m4 = c;
}

// rotate matrix about that axis by ang
// equivalent to post multiplying by the matrix
// made with the make routine
void mx_rot_x_mat_rad(mxs_matrix *dest,const mxs_matrix *m, float ang)
{
   float s,c;
   s=sin(ang); c=cos(ang);
   
   dest->m0 = m->m0;
   dest->m1 = m->m1;
   dest->m2 = m->m2;

   dest->m3 = c*m->m3 + s*m->m6;
   dest->m4 = c*m->m4 + s*m->m7;
   dest->m5 = c*m->m5 + s*m->m8;

   dest->m6 =  -s*m->m3 + c*m->m6;
   dest->m7 =  -s*m->m4 + c*m->m7;
   dest->m8 =  -s*m->m5 + c*m->m8;
}

void mx_rot_y_mat_rad(mxs_matrix *dest,const mxs_matrix *m, float ang)
{
   float s,c;
   s=sin(ang); c=cos(ang);

   dest->m0 = c*m->m0 - s*m->m6;
   dest->m1 = c*m->m1 - s*m->m7;
   dest->m2 = c*m->m2 - s*m->m8;

   dest->m3 = m->m3;
   dest->m4 = m->m4;
   dest->m5 = m->m5;

   dest->m6 = s*m->m0 + c*m->m6;
   dest->m7 = s*m->m1 + c*m->m7;
   dest->m8 = s*m->m2 + c*m->m8;
}

void mx_rot_z_mat_rad(mxs_matrix *dest,const mxs_matrix *m, float ang)
{
   float s,c;
   s=sin(ang); c=cos(ang);

   dest->m6 = m->m6;
   dest->m7 = m->m7;
   dest->m8 = m->m8;

   dest->m0 = c*m->m0 + s*m->m3;
   dest->m1 = c*m->m1 + s*m->m4;
   dest->m2 = c*m->m2 + s*m->m5;

   dest->m3 = -s*m->m0 + c*m->m3;
   dest->m4 = -s*m->m1 + c*m->m4;
   dest->m5 = -s*m->m2 + c*m->m5;
}

// Takes an angvec and constructs an orientation matrix
// from it, in traditional order bank (x), pitch (y), heading (z)
// a.x is bank, a.y is pitch, a.z is heading
void mx_rad2mat(mxs_matrix *dest,const mxs_vector *a)
{
   mxs_matrix m1,m2;

   mx_mk_rot_z_mat_rad(&m1,a->z);
   mx_rot_y_mat_rad(&m2,&m1,a->y);
   mx_rot_x_mat_rad(dest,&m2,a->x);
}

void mx_prn_angvec(const mxs_angvec *a)
{
   printf("tx:%x \tty;%x \ttz:%x\n",a->tx,a->ty,a->tz);
}
