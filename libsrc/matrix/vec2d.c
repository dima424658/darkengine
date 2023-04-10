/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/vec2.c $
 * $Revision: 1.2 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:18 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrixd.h>

#define MXD_VERSION


// Gives you the minimum x,y,z in the dest
void mxd_min_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2)
{
   *dest=*v1;
   mxd_mineq_vec(dest,v2);   
}

void mxd_mineq_vec(mxds_vector *dest,const mxds_vector *v)
{
   if (v->x < dest->x) dest->x = v->x;
   if (v->y < dest->y) dest->y = v->y;
   if (v->z < dest->z) dest->z = v->z;
}

// Gives you the maximum x,y,z in the dst
void mxd_max_vec(mxds_vector *dest,const mxds_vector *v1,const mxds_vector *v2)
{
   *dest=*v1;
   mxd_maxeq_vec(dest,v2);   
}


void mxd_maxeq_vec(mxds_vector *dest,const mxds_vector *v)
{
   if (v->x > dest->x) dest->x = v->x;
   if (v->y > dest->y) dest->y = v->y;
   if (v->z > dest->z) dest->z = v->z;
}


