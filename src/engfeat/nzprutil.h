/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#ifndef __NZPRUTIL_H
#define __NZPRUTIL_H

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

#endif // __NZPRUTIL_H
