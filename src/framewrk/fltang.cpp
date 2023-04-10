/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/fltang.cpp,v 1.14 1998/10/05 17:23:39 mahk Exp $
///////////////////////
//
// fltang.cpp
//
// Briscoe Rodgers
// Looking Glass Technologies, Inc.
// 6/3/96
//
///////////////////////

#include <lg.h>
#include <stdlib.h>
#include <fltang.h>

// must be last header
#include <dbmem.h>


void floatang::set(const float sourceX, const float sourceY, const float endX, const float endY)
{
   double dx, dy;

   dx = endX - sourceX;
   dy = endY - sourceY;

   if (dx != 0 || dy != 0)
   {
      value = atan2(dy, dx); // OPTIMIZE

      // atan 2 returns -pi to pi, so lets normalize

#ifdef STRONG_DEBUG
      AssertMsg4(value >= -(PI + 0.01) && value <= (PI + .01), "atan2() returned Nan on (%f, %f):(%f, %f)", sourceX, sourceY, endX, endY);
#endif

      if (value <= 0)
         value += TWO_PI;

   }
   else
      value = 0;

}
