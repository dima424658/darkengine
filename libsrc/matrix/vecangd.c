/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/vecang.c $
 * $Revision: 1.4 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:25 $
 */

#include <matrixd.h>
#include <_test.h>

#define MXD_VERSION

#define USE_SYMMETRIC_TRIG
#include "mxsymd.h"

// Rotates a vector about the appropriate axis.
// Put in for Sean.  With love, Jaemz
// Assumes src and dest are different
void mxd_rot_x_vec(mxds_vector *dst,const mxds_vector *src,mxs_ang ang)
{
   double s,c;

   TEST_SRCDST("mxd_rot_x_vec",dst,src);

   mxd_sincos(ang,&s,&c);

   dst->x = src->x;
   dst->y = src->y * c - src->z * s;
   dst->z = src->y * s + src->z * c;
}

void mxd_rot_y_vec(mxds_vector *dst,const mxds_vector *src,mxs_ang ang)
{
   double s,c;

   TEST_SRCDST("mxd_rot_y_vec",dst,src);

   mxd_sincos(ang,&s,&c);

   dst->x = src->x * c + src->z * s;
   dst->y = src->y;
   dst->z = -src->x * s + src->z * c;
}

void mxd_rot_z_vec(mxds_vector *dst,const mxds_vector *src,mxs_ang ang)
{
   double s,c;

   TEST_SRCDST("mxd_rot_z_vec",dst,src);

   mxd_sincos(ang,&s,&c);

   dst->x = c * src->x - s * src->y;
   dst->y = s * src->x + c * src->y;
   dst->z = src->z;
}
