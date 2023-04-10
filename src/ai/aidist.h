/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidist.h,v 1.4 1998/07/23 13:14:44 TOML Exp $
//
//
//

#ifndef __AIDIST_H
#define __AIDIST_H

#include <matrixc.h>
#include <fastflts.h>

#pragma once

inline float AIDistance(float x1, float y1, float x2, float y2)
{
   float a, b;

   a = x2 - x1;
   b = y2 - y1;

   a = ffabsf(a);
   b = ffabsf(b);

   if (pflt(b, a))
      return (a + b/2);
   else
      return (b + a/2);
}

inline float AIDistance(const mxs_vector &p1, const mxs_vector &p2)
{
   float a, b, c;

   a = p2.x - p1.x;
   b = p2.y - p1.y;
   c = p2.z - p1.z;

   a = ffabsf(a);
   b = ffabsf(b);
   c = ffabsf(c);

   if (pflt(b, a) && pflt(c, a))
      return (a + b/2 + c/2);

   if (pflt(a, b) && pflt(c, b))
      return (b + a/2 + c/2);

   return (c + a/2 + b/2);
}

inline float AIXYDistance(const mxs_vector &p1, const mxs_vector &p2)
{
   float a, b;

   a = p2.x - p1.x;
   b = p2.y - p1.y;

   a = ffabsf(a);
   b = ffabsf(b);

   if (pflt(b, a))
      return (a + b/2);

   return (b + a/2);
}

inline float AIXYDistanceSq(const mxs_vector &p1, const mxs_vector &p2)
{
   return ((p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y));
}

inline float AIDistanceSq(float x1, float y1, float x2, float y2)
{
   return ((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

inline float AIDistanceSq(const mxs_vector &a, const mxs_vector &b)
{
   return ((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z));
}

///////////////////////////////////////

inline BOOL AIInsideSphere(const mxs_vector & loc, const mxs_vector & center, float radiusSq)
{
   return pflt(AIDistanceSq(loc, center), radiusSq);
}

inline BOOL AIInsideCylinder(const mxs_vector & loc, const mxs_vector & center, float radiusSq, float halfHeight)
{
   if (pflt(ffabsf(center.z -  loc.z), halfHeight))
      return pflt(AIXYDistanceSq(loc, center), radiusSq);
   return FALSE;
}

inline BOOL AIInsideSphere(const mxs_vector & loc, const mxs_vector & center, float radiusSq, float * pDistSq)
{
   *pDistSq = AIDistanceSq(loc, center);
   return pflt(*pDistSq, radiusSq);
}

inline BOOL AIInsideCylinder(const mxs_vector & loc, const mxs_vector & center, float radiusSq, float halfHeight, float * pDistSq)
{
   *pDistSq = AIDistanceSq(loc, center);
   if (pflt(ffabsf(center.z -  loc.z), halfHeight))
      return pflt(AIXYDistanceSq(loc, center), radiusSq);
   return FALSE;
}

///////////////////////////////////////

#endif /* !__AIDIST_H */
