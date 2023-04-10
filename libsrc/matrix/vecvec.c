/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/vecvec.c $
 * $Revision: 1.8 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:33 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrix.h>

#define MX_VERSION

// v = 0
void mx_zero_vec(mxs_vector *v)
{
   v->x = 0;
   v->y = 0;
   v->z = 0;
}

// create a unit vec in row n
void mx_unit_vec(mxs_vector *v,int n)
{
   v->x = 0;
   v->y = 0;
   v->z = 0;
   v->el[n] = 1.0;
}

// copy a vec
void mx_copy_vec(mxs_vector *dest,const mxs_vector *src)
{
   memcpy(dest,src,sizeof(mxs_vector));
}

// dest = v1 + v2;
void mx_add_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2)
{
   dest->x = v1->x + v2->x;
   dest->y = v1->y + v2->y;
   dest->z = v1->z + v2->z;
}

// v1 += v2;
void mx_addeq_vec(mxs_vector *v1,const mxs_vector *v2)
{
   v1->x += v2->x;
   v1->y += v2->y;
   v1->z += v2->z;
}

// dest = v1-v2
void mx_sub_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2)
{
   dest->x = v1->x - v2->x;
   dest->y = v1->y - v2->y;
   dest->z = v1->z - v2->z;
}

// v1 -= v2;
void mx_subeq_vec(mxs_vector *v1,const mxs_vector *v2)
{
   v1->x -= v2->x;
   v1->y -= v2->y;
   v1->z -= v2->z;
}

// dest = s * v
void mx_scale_vec(mxs_vector *dest,const mxs_vector *v,mxs_real s)
{
   dest->x = v->x * s;
   dest->y = v->y * s;
   dest->z = v->z * s;
}

// v *= s
void mx_scaleeq_vec(mxs_vector *v,mxs_real s)
{
   v->x *= s;
   v->y *= s;
   v->z *= s;
}

// dest = v1 + s * v2
void mx_scale_add_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2, mxs_real s)
{
   dest->x = v1->x + v2->x * s;
   dest->y = v1->y + v2->y * s;
   dest->z = v1->z + v2->z * s;
}

// v1 += v2*s
void mx_scale_addeq_vec(mxs_vector *v1,const mxs_vector *v2,mxs_real s)
{
   v1->x += v2->x * s;
   v1->y += v2->y * s;
   v1->z += v2->z * s;
}

// dest = v1*(1-s) + v2*(s)
void mx_interpolate_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2,mxs_real s)
{
   dest->x = v1->x*(1-s) + v2->x*s;
   dest->y = v1->y*(1-s) + v2->y*s;
   dest->z = v1->z*(1-s) + v2->z*s;
}

// dest = v /s
void mx_div_vec(mxs_vector *dest,const mxs_vector *v,mxs_real s)
{
   dest->x = v->x/s;
   dest->y = v->y/s;
   dest->z = v->z/s;
}

// v /= s
void mx_diveq_vec(mxs_vector *v,mxs_real s)
{
   v->x /= s;
   v->y /= s;
   v->z /= s;
}  

// dest = v1 .* v2      // matlab notation
void mx_elmul_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2)
{
   dest->x = v1->x * v2->x;
   dest->y = v1->y * v2->y;
   dest->z = v1->z * v2->z;
}

// v1 .*= v2
void mx_elmuleq_vec(mxs_vector *v1,const mxs_vector *v2)
{
   v1->x *= v2->x;
   v1->y *= v2->y;
   v1->z *= v2->z;
}

// |v1-v2|^2
mxs_real mx_dist2_vec(const mxs_vector *v1,const mxs_vector *v2)
{
   mxs_vector tmp;

   tmp.x = v2->x - v1->x;
   tmp.y = v2->y - v1->y;
   tmp.z = v2->z - v1->z;
   return mx_mag2_vec(&tmp);
}

// |v1-v2|
mxs_real mx_dist_vec(const mxs_vector *v1,const mxs_vector *v2)
{
   mxs_vector tmp;

   tmp.x = v2->x - v1->x;
   tmp.y = v2->y - v1->y;
   tmp.z = v2->z - v1->z;
   return mx_mag_vec(&tmp);
}


// |v|^2
mxs_real mx_mag2_vec(const mxs_vector *v)
{
   return v->x*v->x + v->y*v->y + v->z*v->z;   
}

// |v|
mxs_real mx_mag_vec(const mxs_vector *v)
{
   mxs_real d;
   d = v->x*v->x + v->y*v->y + v->z*v->z;   
   return sqrt(d);
}

// dest = v1/|v1|, returns |v1|
mxs_real mx_norm_vec(mxs_vector *dest,const mxs_vector *v)
{
   mxs_real d,id;

   d = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
   id = 1.0/d;
   dest->x = v->x*id;
   dest->y = v->y*id;
   dest->z = v->z*id;
   return d;
}

// v1 /= |v1|, return |v1|
mxs_real mx_normeq_vec(mxs_vector *v)
{
   mxs_real d,id;

   d = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
   id = 1.0/d;
   v->x *= id;
   v->y *= id;
   v->z *= id;
   return d;
}

// return v1 . v2
mxs_real mx_dot_vec(const mxs_vector *v1,const mxs_vector *v2)
{
   return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}

// dest = v1 x v2
void mx_cross_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2)
{
   dest->x = v1->y * v2->z - v1->z*v2->y;
   dest->y = v1->z * v2->x - v1->x*v2->z;
   dest->z = v1->x * v2->y - v1->y*v2->x;
}

// dest = (v1 x v2) / |v1 x v2|, return |v1 x v2|
mxs_real mx_cross_norm_vec(mxs_vector *dest,const mxs_vector *v1, const mxs_vector *v2)
{
   dest->x = v1->y * v2->z - v1->z*v2->y;
   dest->y = v1->z * v2->x - v1->x*v2->z;
   dest->z = v1->x * v2->y - v1->y*v2->x;
   return mx_normeq_vec(dest);
}

// dest = -v
void mx_neg_vec(mxs_vector *dest,const mxs_vector *v)
{
   dest->x = -v->x;
   dest->y = -v->y;
   dest->z = -v->z;
}
// v = -v
void mx_negeq_vec(mxs_vector *v)
{
   v->x = -v->x;
   v->y = -v->y;
   v->z = -v->z;
}

void mx_prn_vec(const mxs_vector *v)
{
   printf("%f \t%f \t%f\n",v->x,v->y,v->z);
}
