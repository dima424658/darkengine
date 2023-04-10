/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkprutl.h,v 1.3 2000/01/31 09:58:43 adurant Exp $
#pragma once

This file has been moved to nzprutil.h AMSD

#ifndef __SHKPRUTL_H
#define __SHKPRUTL_H

///////////////////////////////////////////////////////////////////////////////
//
// Handy way to specifify nozeroing without having a seperate class for 
// every property
//

#ifdef __DATAOPS__H

template <class TYPE> 
class cNoZeroDataOps : public cClassDataOps<TYPE>
{
public:
   cNoZeroDataOps()
    : cClassDataOps<TYPE>(cClassDataOps<TYPE>::kNoFlags)
   {
   }
}; 

#else

class cNoZeroDataOps;

#endif // __DATAOPS_H

#endif // __SHKPRUTL_H
