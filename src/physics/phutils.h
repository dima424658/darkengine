/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phutils.h,v 1.9 2000/01/31 09:52:11 adurant Exp $
//
//
// utility functions, mainly vector
//
#pragma once

#ifndef __PHUTILS_H
#define __PHUTILS_H

#include <matrixs.h>
#include <mprintf.h>

struct tConstraint;

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

///////////////////////////////////////////////////////////////////////////////
//
// remove component along normal(s), set subVel to the removed component
//
extern void PhysRemNormComp(mxs_vector *vec, const mxs_vector *constraint_list, int size);
extern void PhysRemNormComp(mxs_vector *vec, const mxs_vector &constraint);
extern void PhysRemNormComp(mxs_vector *vec, const mxs_vector  &constraint1, const mxs_vector &constraint2);

extern void PhysConstrain(mxs_vector *vec, const tConstraint *constraint_list, int size);
extern void PhysConstrain(mxs_vector *vec, const tConstraint &constraint);
extern void PhysConstrian(mxs_vector *vec, const tConstraint &constraint1, const tConstraint &constraint2);

///////////////////////////////////////////////////////////////////////////////
//
// solve for a line distance to point, line parameterized as p1+t*v1, point is p2
// return smallest t for which distance = r
//

extern BOOL PhysSolveLnPtDistVec(const mxs_vector * p1, const mxs_vector * delta,
                                 const mxs_vector * p2, mxs_real r,
                                 mxs_real dt, mxs_real * t);

extern BOOL PhysSolveLnPtDist(const mxs_vector * p1, const mxs_vector * v1,
                              const mxs_vector * p2, mxs_real r,
                              mxs_real dt, mxs_real * t);

///////////////////////////////////////////////////////////////////////////////
//
// are we a zero vector?
//

inline BOOL IsZeroVector(const mxs_vector & v)
{
   return (mx_mag2_vec(&v) < 0.0001);
}

///////////////////////////////////////////////////////////////////////////////
//
// spew the vector
//

inline void MonoPrintVector(const mxs_vector & v)
{
   mprintf("(0 0 0)\n", v.x, v.y, v.z);
}

///////////////////////////////////////////////////////////////////////////////

inline BOOL EqualVectors(const mxs_vector & v1, const mxs_vector & v2)
{
   return ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z));
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHUTILS_H */
