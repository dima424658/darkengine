/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/vec2.c $
 * $Revision: 1.2 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:18 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrix.h>

#define MX_VERSION


// Gives you the minimum x,y,z in the dest
void mx_min_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2)
{
   *dest=*v1;
   mx_mineq_vec(dest,v2);   
}

void mx_mineq_vec(mxs_vector *dest,const mxs_vector *v)
{
   if (v->x < dest->x) dest->x = v->x;
   if (v->y < dest->y) dest->y = v->y;
   if (v->z < dest->z) dest->z = v->z;
}

// Gives you the maximum x,y,z in the dst
void mx_max_vec(mxs_vector *dest,const mxs_vector *v1,const mxs_vector *v2)
{
   *dest=*v1;
   mx_maxeq_vec(dest,v2);   
}


void mx_maxeq_vec(mxs_vector *dest,const mxs_vector *v)
{
   if (v->x > dest->x) dest->x = v->x;
   if (v->y > dest->y) dest->y = v->y;
   if (v->z > dest->z) dest->z = v->z;
}


