/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/vecvec.c $
 * $Revision: 1.8 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:33 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrixd.h>

#define MXD_VERSION

// v = 0
void mxd_zero_vec(mxds_vector *v)
{
   v->x = 0;
   v->y = 0;
   v->z = 0;
}

// create a unit vec in row n
void mxd_unit_vec(mxds_vector *v,int n)
{
   v->x = 0;
   v->y = 0;
   v->z = 0;
   v->el[n] = 1.0;
}

// copy a vec
void mxd_copy_vec(mxds_vector *dest,const mxds_vector *src)
{
   memcpy(dest,src,sizeof(mxds_vector));
}

// dest = v1 + v2;
void mxd_add_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2)
{
   dest->x = v1->x + v2->x;
   dest->y = v1->y + v2->y;
   dest->z = v1->z + v2->z;
}

// v1 += v2;
void mxd_addeq_vec(mxds_vector *v1,const mxds_vector *v2)
{
   v1->x += v2->x;
   v1->y += v2->y;
   v1->z += v2->z;
}

// dest = v1-v2
void mxd_sub_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2)
{
   dest->x = v1->x - v2->x;
   dest->y = v1->y - v2->y;
   dest->z = v1->z - v2->z;
}

// v1 -= v2;
void mxd_subeq_vec(mxds_vector *v1,const mxds_vector *v2)
{
   v1->x -= v2->x;
   v1->y -= v2->y;
   v1->z -= v2->z;
}

// dest = s * v
void mxd_scale_vec(mxds_vector *dest,const mxds_vector *v,double s)
{
   dest->x = v->x * s;
   dest->y = v->y * s;
   dest->z = v->z * s;
}

// v *= s
void mxd_scaleeq_vec(mxds_vector *v,double s)
{
   v->x *= s;
   v->y *= s;
   v->z *= s;
}

// dest = v1 + s * v2
void mxd_scale_add_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2, double s)
{
   dest->x = v1->x + v2->x * s;
   dest->y = v1->y + v2->y * s;
   dest->z = v1->z + v2->z * s;
}

// v1 += v2*s
void mxd_scale_addeq_vec(mxds_vector *v1,const mxds_vector *v2,double s)
{
   v1->x += v2->x * s;
   v1->y += v2->y * s;
   v1->z += v2->z * s;
}

// dest = v1*(1-s) + v2*(s)
void mxd_interpolate_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2,double s)
{
   dest->x = v1->x*(1-s) + v2->x*s;
   dest->y = v1->y*(1-s) + v2->y*s;
   dest->z = v1->z*(1-s) + v2->z*s;
}

// dest = v /s
void mxd_div_vec(mxds_vector *dest,const mxds_vector *v,double s)
{
   dest->x = v->x/s;
   dest->y = v->y/s;
   dest->z = v->z/s;
}

// v /= s
void mxd_diveq_vec(mxds_vector *v,double s)
{
   v->x /= s;
   v->y /= s;
   v->z /= s;
}  

// dest = v1 .* v2      // matlab notation
void mxd_elmul_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2)
{
   dest->x = v1->x * v2->x;
   dest->y = v1->y * v2->y;
   dest->z = v1->z * v2->z;
}

// v1 .*= v2
void mxd_elmuleq_vec(mxds_vector *v1,const mxds_vector *v2)
{
   v1->x *= v2->x;
   v1->y *= v2->y;
   v1->z *= v2->z;
}

// |v1-v2|^2
double mxd_dist2_vec(const mxds_vector *v1,const mxds_vector *v2)
{
   mxds_vector tmp;

   tmp.x = v2->x - v1->x;
   tmp.y = v2->y - v1->y;
   tmp.z = v2->z - v1->z;
   return mxd_mag2_vec(&tmp);
}

// |v1-v2|
double mxd_dist_vec(const mxds_vector *v1,const mxds_vector *v2)
{
   mxds_vector tmp;

   tmp.x = v2->x - v1->x;
   tmp.y = v2->y - v1->y;
   tmp.z = v2->z - v1->z;
   return mxd_mag_vec(&tmp);
}


// |v|^2
double mxd_mag2_vec(const mxds_vector *v)
{
   return v->x*v->x + v->y*v->y + v->z*v->z;   
}

// |v|
double mxd_mag_vec(const mxds_vector *v)
{
   double d;
   d = v->x*v->x + v->y*v->y + v->z*v->z;   
   return sqrt(d);
}

// dest = v1/|v1|, returns |v1|
double mxd_norm_vec(mxds_vector *dest,const mxds_vector *v)
{
   double d,id;

   d = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
   id = 1.0/d;
   dest->x = v->x*id;
   dest->y = v->y*id;
   dest->z = v->z*id;
   return d;
}

// v1 /= |v1|, return |v1|
double mxd_normeq_vec(mxds_vector *v)
{
   double d,id;

   d = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
   id = 1.0/d;
   v->x *= id;
   v->y *= id;
   v->z *= id;
   return d;
}

// return v1 . v2
#ifndef _MSC_VER
double mxd_dot_vec(const mxds_vector *v1,const mxds_vector *v2)
{
   return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}
#endif

// dest = v1 x v2
void mxd_cross_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2)
{
   dest->x = v1->y * v2->z - v1->z*v2->y;
   dest->y = v1->z * v2->x - v1->x*v2->z;
   dest->z = v1->x * v2->y - v1->y*v2->x;
}

// dest = (v1 x v2) / |v1 x v2|, return |v1 x v2|
double mxd_cross_norm_vec(mxds_vector *dest,const mxds_vector *v1, const mxds_vector *v2)
{
   dest->x = v1->y * v2->z - v1->z*v2->y;
   dest->y = v1->z * v2->x - v1->x*v2->z;
   dest->z = v1->x * v2->y - v1->y*v2->x;
   return mxd_normeq_vec(dest);
}

// dest = -v
void mxd_neg_vec(mxds_vector *dest,const mxds_vector *v)
{
   dest->x = -v->x;
   dest->y = -v->y;
   dest->z = -v->z;
}
// v = -v
void mxd_negeq_vec(mxds_vector *v)
{
   v->x = -v->x;
   v->y = -v->y;
   v->z = -v->z;
}

void mxd_prn_vec(const mxds_vector *v)
{
   printf("%f \t%f \t%f\n",v->x,v->y,v->z);
}
