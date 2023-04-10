/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/framewrk/fastflts.h,v 1.3 1998/05/22 19:31:06 TOML Exp $
//
//
//

#ifndef __FASTFLTS_H
#define __FASTFLTS_H

#pragma once

#ifdef __cplusplus

inline float ffabsf(float f)
{
   *((unsigned *)&f) &= 0x7fffffff;
   return f;
}

inline BOOL afle(const float & l, const float & r)
{
   return (((* ((const int *)(&(l)))) & 0x7fffffff) <= ((*((const int *)&r)) & 0x7fffffff));
}

inline BOOL pfle(const float & l, const float & r) // Be careful, zero can be negative
{
   return (((*((const int *)(&(l))))) <= (*((const int *)&r)));
}

inline BOOL aflt(const float & l, const float & r)
{
   return (((* ((const int *)(&(l)))) & 0x7fffffff) < ((*((const int *)&r)) & 0x7fffffff));
}

inline BOOL pflt(const float & l, const float & r) // Be careful, zero can be negative
{
   return (((*((const int *)(&(l))))) < (*((const int *)&r)));
}

inline BOOL pflcmp(const float & l, const float & r) // Be careful, zero can be negative
{
   return (((*((const int *)(&(l))))) - (*((const int *)&r)));
}

#endif

#endif /* !__FASTFLTS_H */
