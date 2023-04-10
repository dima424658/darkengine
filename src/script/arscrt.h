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
   long id; 
public:
   reaction_kind(long i) : id(i) {};
   reaction_kind(const char* name); 

   operator long() const { return id;}; 
   operator string() const;

}; 



#endif // __ARSCRT_H
