/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phutils.cpp,v 1.22 2000/02/19 12:32:25 toml Exp $
//
//
//

#include <lg.h>
#include <stdlib.h>
#include <math.h>
#include <matrix.h>
#include <matrixs.h>
#include <phmod.h>
#include <phutils.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#undef rad2

///////////////////////////////////////////////////////////////////////////////

#define MAX_NUM_CONSTRAINTS  (12)

void PhysRemNormComp(mxs_vector *vec, const mxs_vector *norm_list, int size)
{
   mxs_vector orig;
   mxs_vector inter;
   int count = 0;
   int i;

   #ifndef SHIP
   BOOL fully_constrained = FALSE;
   #endif

   if (size > MAX_NUM_CONSTRAINTS)
      Warning(("PhysRemNormComp: too many constraints: %d, only using first %d\n", size, MAX_NUM_CONSTRAINTS));

   count = (size > MAX_NUM_CONSTRAINTS) ? MAX_NUM_CONSTRAINTS : size;
   mx_copy_vec(&orig, vec);

   // First, special-case the two-vector case
   int real_count = 0;
   int real_norm_1 = -1;
   int real_norm_2 = -1;
   for (i = 0; i < count; i++)
   {
      if (mx_dot_vec(vec, &norm_list[i]) <= 0.0)
      {
         real_count++;

         if (real_count > 2)
            break;

         if (real_norm_1 == -1)
            real_norm_1 = i;
         else
            real_norm_2 = i;
      }
   }

   if (real_count == 2)
   {
      mxs_vector test_vec;

      test_vec = *vec;
      PhysRemNormComp(&test_vec, norm_list[real_norm_1]);

      if (mx_dot_vec(&test_vec, &norm_list[real_norm_2]) <= 0.0)
      {
         test_vec = *vec;
         PhysRemNormComp(&test_vec, norm_list[real_norm_2]);

         if (mx_dot_vec(&test_vec, &norm_list[real_norm_1]) <= 0.0)
         {
            PhysRemNormComp(vec, norm_list[real_norm_1], norm_list[real_norm_2]);
            return;
         }
      }
   }

   // Do constraint pass
   for (i=0; i<count; i++)
   {
      if (mx_dot_vec(vec, &norm_list[i]) <= 0.0)
         PhysRemNormComp(vec, norm_list[i]);
   }

   // Save off intermediate vector
   mx_copy_vec(&inter, vec);

   // Check if the result is actually pointing us backward
   if (mx_dot_vec(&inter, &orig) < 0.0)
   {
      mx_zero_vec(vec);

      #ifndef SHIP
      fully_constrained = TRUE;
      #endif
   }

   // Now dot against all our constraints to make sure we're not still going in
   for (i=0; i<count; i++)
   {
      mxs_real dot = mx_dot_vec(vec, &norm_list[i]);
      if (dot < -0.0001)
      {
         mx_zero_vec(vec);
         break;
      }
   }
}

void PhysRemNormComp(mxs_vector *vec, const mxs_vector &norm)
{
   mxs_vector normal;

   // remove all vec components normal to surface
   mx_scale_vec(&normal, (mxs_vector *)&norm, min(0, mx_dot_vec(vec, (mxs_vector *)&norm)));
   mx_subeq_vec(vec, &normal);
}

void PhysRemNormComp(mxs_vector *vec, const mxs_vector &norm1, const mxs_vector &norm2)
{
   mxs_vector cross_prod;
   mxs_vector component;

   // remove all vec components within plane defined by norm1 and norm2
   mx_cross_vec(&cross_prod, (mxs_vector *)&norm1, (mxs_vector *)&norm2);

   if (IsZeroVector(cross_prod))
   {
      PhysRemNormComp(vec, norm1);
      PhysRemNormComp(vec, norm2);
   }
   else
   {
      mx_normeq_vec(&cross_prod);
      mx_scale_vec(&component, &cross_prod, mx_dot_vec(vec, &cross_prod));
      mx_copy_vec(vec, &component);
   }
}

///////////////////////////////////////////////////////////////////////////////

void PhysConstrain(mxs_vector *vec, const tConstraint *constraint_list, int size)
{
   mxs_vector vec_list[MAX_NUM_CONSTRAINTS];

   for (int i=0; i<size && i<MAX_NUM_CONSTRAINTS; i++)
      vec_list[i] = constraint_list[i].dir;

   PhysRemNormComp(vec, vec_list, size);
}

void PhysConstrain(mxs_vector *vec, const tConstraint &constraint)
{
   PhysRemNormComp(vec, constraint.dir);
}

void PhysConstrian(mxs_vector *vec, const tConstraint &constraint1, const tConstraint &constraint2)
{
   PhysRemNormComp(vec, constraint1.dir, constraint2.dir);
}

///////////////////////////////////////////////////////////////////////////////
//
// Solve for the first point on the parametric line p1+t*v1 whose distance to p2 is r. If such
// a point can be found then return TRUE and set t.
//

BOOL PhysSolveLnPtDistVec(const mxs_vector * p1, const mxs_vector * delta,
                        const mxs_vector * p2, mxs_real r, 
                        mxs_real dt, mxs_real * t)
{
   mxs_vector vel;
   mx_scale_vec(&vel, delta, 1 / dt);

   return PhysSolveLnPtDist(p1, &vel, p2, r, dt, t);
}

BOOL PhysSolveLnPtDist(const mxs_vector * p1, const mxs_vector * v1,
                       const mxs_vector * p2, mxs_real r,
                       mxs_real dt, mxs_real * t)
{
   mxs_vector v1_true;
   mxs_vector v1_norm;
   mxs_vector v12, v23;
   mxs_vector p3;
   mxs_real   v12_proj;
   mxs_real   rad_proj;
   mxs_real   v1_mag;
   mxs_real   v23_mag2;
   mxs_real   rad2 = (r * r);

   // Setup
   mx_scale_vec(&v1_true, (mxs_vector *)v1, dt);
   mx_sub_vec(&v12, (mxs_vector *)p2, (mxs_vector *)p1);
   mx_norm_vec(&v1_norm, &v1_true);

   // Find length of projection of v12 onto v1
   v12_proj = mx_dot_vec(&v12, &v1_norm);

   // Find point along v1 to which v2 projects
   mx_scale_add_vec(&p3, (mxs_vector *)p1, &v1_norm, v12_proj);

   // Vector from p2 to p3, which is min dist of p2 from v1
   mx_sub_vec(&v23, &p3, (mxs_vector *)p2);

   // Reject if out of range
   v23_mag2 = mx_mag2_vec(&v23);
   if (v23_mag2 > rad2)
      return FALSE;

   // Find length of projection of radius (from p2) onto v1 using pythagorean theorum
   rad_proj = sqrt(rad2 - v23_mag2);
   
   v1_mag = mx_mag_vec(&v1_true);

   // Find portion of v1 that is before the intersection, and scale
   // the timeslice by that
   *t = dt * ((v12_proj - rad_proj) / v1_mag);
   return TRUE;
}

#if 0
   mxs_real    v1MagSq  = mx_mag2_vec((mxs_vector *) v1);
   mxs_vector  v12,
               v24;
   mxs_real    d13Sq,
               d14Sq,
               d24Sq;
   mxs_real    rSq      = r * r;
   mxs_real    dpv12v1;
   mxs_vector  p4;

   // Find the square of the length of the projection of v12 onto v1 
   mx_sub_vec(&v12, (mxs_vector *) p2, (mxs_vector *) p1);
   dpv12v1 = mx_dot_vec(&v12, (mxs_vector *) v1);
   d14Sq   = (dpv12v1 * dpv12v1) / v1MagSq;

   // Add to find point of projection
   mx_scale_add_vec(&p4, (mxs_vector *) p1, (mxs_vector *) v1, sqrt(d14Sq / v1MagSq));

   // Create vector from p2 normal to v1
   mx_sub_vec(&v24, &p4, (mxs_vector *) p2);
   d24Sq   = mx_mag2_vec(&v24);

   // Reject if too far away
   if (d24Sq > rSq)
      return FALSE;

   // Find the square of the length of the projection of radius onto v1 (pythag)
   d13Sq = rSq - d24Sq;

   // Determine portion along v1 that d14 - d13 represents, and scale
   // timeslice accordingly.  Uses ((a - b) / c) ^ 2 == (a^2 - 2ab + b^2) / c^2.
   *t = dt * sqrt((d14Sq - (2 * sqrt(d14Sq) * sqrt(d13Sq)) + d13Sq) / v1MagSq);

   return TRUE;
}

#endif
///////////////////////////////////////////////////////////////////////////////



