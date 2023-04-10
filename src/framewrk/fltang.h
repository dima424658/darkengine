/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/fltang.h,v 1.14 2000/01/29 13:20:52 adurant Exp $
///////////////////////
//
// fltang.h
//
// Briscoe Rodgers
// Looking Glass Technologies, Inc.
// 6/3/96
//
///////////////////////
#pragma once

#ifndef _FLTANG_H
#define _FLTANG_H

#include <float.h>
#include <math.h>
#include <fastflts.h>

#define PI     3.141592653589793238
#define TWO_PI 6.283185307178

// To convert from degrees to radians
#define DEGREES(x) ((((float)(x))*TWO_PI)/360.0)
#define RADIANS(x) ((((float)(x))*360.0)/TWO_PI)

///////////////////////////////////////////////////////////////////////////////

class floatang
{
   public:
      float  value;

             floatang();
             floatang(const floatang & v);
             floatang(const float v);
             floatang(const float sourceX, const float sourceY, const float endX, const float endY);

      friend floatang  operator+(const floatang&, const floatang&);
      friend floatang  operator-(const floatang&, const floatang&);
      friend floatang  operator*(const floatang&, const floatang&);
      friend floatang  operator/(const floatang&, const floatang&);

      friend int       operator==(const floatang& a, const floatang& b);
      friend int       operator!=(const floatang& a, const floatang& b);
      friend int       operator<(const floatang& a, const floatang& b);
      friend int       operator>(const floatang& a, const floatang& b);
      friend int       operator<=(const floatang& a, const floatang& b);
      friend int       operator>=(const floatang& a, const floatang& b);

             int       between(const floatang& ccw, const floatang& cw) const;
             int       cwFrom(const floatang& a) const;  // am I closer to clockwise from angle a? TRUE when equal.
             int       ccwFrom(const floatang& a) const; // am I closer to clockwise from angle a? TRUE when equal.

             void      set(const float sourceX, const float sourceY, const float endX, const float endY);
};

floatang floatang_middleOf2Angs(const floatang& a, const floatang& b);
//floatang floatang_smallestDeltaOf2Angs(const floatang& a, const floatang& b);
floatang Delta(const floatang& a, const floatang& b);


///////////////////////////////////////////////////////////////////////////////

class floatarc
{
   public:
      floatang  ccw, cw;

                floatarc();
                floatarc(const floatang& ccwPost, const floatang& cwPost);

      friend    floatarc operator+(const floatarc&, const floatarc&);
      friend    floatarc operator-(const floatarc&, const floatarc&);

      int       operator==(const floatarc& a);
      int       operator!=(const floatarc& a);

      floatang  center() const;
      floatang  span() const;

      int       contains(const floatarc& arc) const; // is arc in *this? inclusive.
      int       contains(const floatang& ang) const; // is ang in *this? inclusive.

      void      inverse();
      void      SetByCenterAndSpan(floatang arcCenter, floatang arcSpan);
};

///////////////////////////////////////////////////////////////////////////////

// 0 to 2PI
#define floatang_normalize_angle(v) \
   if ((v) < 0)                 \
   {                            \
      do                        \
      {                         \
         (v) += TWO_PI;         \
      }                         \
      while ((v) <= 0);         \
                                \
      if ((v) > TWO_PI)         \
      {                         \
         (v) = TWO_PI;          \
      }                         \
   }                            \
   else if ((v)>0)              \
   {                            \
      while (pfle(TWO_PI, (v))) \
      {                         \
         (v) -= TWO_PI;         \
      }                         \
                                \
   }

inline floatang::floatang()
 : value(0)
{
}

inline floatang::floatang(const float v)
 : value(v)
{
#ifdef STRONG_DEBUG
   float v2 = v;
   floatang_normalize_angle(v2);
   AssertMsg(v == 0 || v2 == v, "initial floatang value must be normalized");
#endif
}

inline floatang::floatang(const floatang & v)
 : value(v.value)
{
#ifdef STRONG_DEBUG
   float v2 = v;
   floatang_normalize_angle(v2);
   AssertMsg(v == 0 || v2 == v, "initial floatang value must be normalized");
#endif
}

inline floatang::floatang(const float sourceX, const float sourceY, const float endX, const float endY)
{
   set(sourceX, sourceY, endX, endY);
}

inline floatang operator+(const floatang& a, const floatang& b)
{
   float v = a.value + b.value;

   floatang_normalize_angle(v);

   return v;
}

inline floatang operator-(const floatang& a, const floatang& b)
{
   float v = a.value - b.value;

   floatang_normalize_angle(v);

   return v;
}

inline floatang operator*(const floatang& a, const floatang& b)
{
   float v = a.value * b.value;

   floatang_normalize_angle(v);

   return v;
}

inline floatang operator/(const floatang& a, const floatang& b)
{
   float v = a.value / b.value;

   floatang_normalize_angle(v);

   return v;
}

inline int operator==(const floatang& a, const floatang& b)
{
   return a.value == b.value;
}

inline int operator!=(const floatang& a, const floatang& b)
{
   return a.value != b.value;
}

inline int operator<(const floatang& a, const floatang& b)
{
   return pflt(a.value, b.value);
}

inline int operator>(const floatang& a, const floatang& b)
{
   return pflt(b.value, a.value);
}

inline int operator<=(const floatang& a, const floatang& b)
{
   return pfle(a.value, b.value);
}

inline int operator>=(const floatang& a, const floatang& b)
{
   return pfle(b.value, a.value);
}

inline int floatang::cwFrom(const floatang& a) const
{
   return (*this - a) >= (a - *this);
}

inline int floatang::ccwFrom(const floatang& a) const
{
   return (*this - a) <= (a - *this);
}

// @TBD (toml 09-06-97): optimize to v = fabs(a-b)  0i, return (v > pi) ? 2pi - v : v; ?
inline floatang Delta(const floatang& a, const floatang& b)
{
   const floatang ab(a - b);
   const floatang ba(b - a);

   if (ab < ba)
      return ab;

   return ba;
}

#define floatang_smallestDeltaOf2Angs(a, b) Delta(a, b)

// returns the "middle" of two floatangs...with the middle assumed to be
// the middle of the smaller angle created by the a & b.
inline floatang floatang_middleOf2Angs(const floatang& a, const floatang& b)
{
   return (a + floatang_smallestDeltaOf2Angs(a, b) / 2); 
}

inline int floatang::between(const floatang& ccw, const floatang& cw) const
{
   return ((ccw < cw && *this > ccw && *this < cw) || (ccw > cw && (*this > ccw || *this < cw)));
}

///////////////////////////////////////////////////////////////////////////////

inline floatarc::floatarc()
 : ccw(0), cw(0)
{
}

inline floatarc::floatarc(const floatang& ccwPost, const floatang& cwPost)
 : cw(cwPost), ccw(ccwPost)
{
}

inline int operator==(const floatarc& a, const floatarc& b)
{
   return (a.ccw == b.ccw && a.cw == b.cw);
}

inline int operator!=(const floatarc& a, const floatarc& b)
{
   return (a.ccw != b.ccw || a.cw != b.cw);
}

inline floatang floatarc::center() const
{
   return cw + (ccw - cw) / 2; // @Q (toml 09-11-97): couldn't we just normalize once here for the math ops?
}

inline floatang floatarc::span() const
{
   return ccw - cw;
}

inline floatarc operator+(const floatarc& a, const floatarc& b)
{
   return floatarc(a.ccw + b.ccw, a.cw + b.cw);
}

inline floatarc operator-(const floatarc& a, const floatarc& b)
{
   return floatarc(a.ccw - b.ccw, a.cw - b.cw);
}

inline void floatarc::SetByCenterAndSpan(floatang arcCenter, floatang arcSpan)
{
   cw = arcCenter - (arcSpan / 2); // @Q (toml 09-11-97): couldn't we just normalize once here for the math ops?
   ccw = arcCenter + (arcSpan / 2);
}

inline void floatarc::inverse()
{
   floatang temp;

   temp = ccw;
   ccw = cw;
   cw = temp;
}

inline int floatarc::contains(const floatang& ang) const
{
   if (ccw > cw)
   {
      // normal
      return (ang <= ccw && ang >= cw);
   }
   else
   {
      // around the bend
      return (ang <= ccw || ang >= cw);
   }
}

inline int floatarc::contains(const floatarc& arc) const
{
   return (contains(arc.cw) && contains(arc.ccw));
}

///////////////////////////////////////////////////////////////////////////////

#endif
