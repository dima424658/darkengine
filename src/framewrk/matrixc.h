/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/framewrk/matrixc.h,v 1.2 1998/07/21 13:58:30 TOML Exp $
//
// Matrix as class
//

#ifndef __MATRIXC_H
#define __MATRIXC_H

#pragma once

#include <float.h>

#include <fastflts.h>
#include <matrix.h>
#include <matrixs.h>

#ifdef __cplusplus

typedef struct Vec2 Vec2;

#define kMatrixcEpsilon ((float)(0.000001))

///////////////////////////////////////////////////////////////////////////////

class cMxsVector : public mxs_vector
{
public:

   cMxsVector()                                       {}
   cMxsVector(float xx, float yy, float zz)           { x = xx;  y = yy;  z = zz; }
   cMxsVector(const mxs_vector & v)                   { x = v.x; y = v.y; z = v.z;}
   const cMxsVector & operator=(const mxs_vector &v)  { x = v.x; y = v.y; z = v.z; return *this;}

   void Set(float xx, float yy, float zz)             { x = xx;  y = yy;  z = zz; }
   void Set(const mxs_vector & v)                     { x = v.x; y = v.y; z = v.z;}

   int IsIdent(const mxs_vector& v) const             { return (x == v.x && y == v.y && z == v.z); }

   int operator==(const mxs_vector& a) const          { return ((afle(a.x-x, kMatrixcEpsilon)) &&
                                                                (afle(a.y-y, kMatrixcEpsilon)) &&
                                                                (afle(a.z-z, kMatrixcEpsilon))); }

   int operator!=(const mxs_vector& a) const          { return ((aflt(kMatrixcEpsilon, a.x-x)) ||
                                                                (aflt(kMatrixcEpsilon, a.y-y)) ||
                                                                (aflt(kMatrixcEpsilon, a.z-z))); }

   Vec2 * AsVec2Ptr() const                           { return (Vec2 *)this; }   // @Note (toml 08-22-97): note a Vec2 must be laid out exactly like an mxs_vector for this to work, also, vec2 lib should take const args where meant

   BOOL IsValid() const                               { return !(x == FLT_MAX && y == FLT_MAX && z == FLT_MAX); }
};

///////////////////////////////////////////////////////////////////////////////

#endif

#endif /* !__MATRIXC_H */
