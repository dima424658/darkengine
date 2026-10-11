/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/arscrt.h,v 1.1 1997/12/30 17:50:53 mahk Exp $
#pragma once  
#ifndef __ARSCRT_H
#define __ARSCRT_H

#include <objscrt.h>

typedef object stimulus_kind; 

//
// "reaction type" can be either a string or an id
//
class reaction_kind 
{
   int32 id;
public:
   reaction_kind(int32 i) : id(i) {};
   reaction_kind(const char* name); 

   operator int32() const { return id;};
   operator string() const;

};

static_assert(sizeof(reaction_kind) == 4, "Script reaction IDs require the 32-bit ABI");



#endif // __ARSCRT_H
