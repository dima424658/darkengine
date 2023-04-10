/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propknow.h,v 1.1 1998/03/26 13:52:40 mahk Exp $
#pragma once  
#ifndef __PROPKNOW_H
#define __PROPKNOW_H

#include <propman.h>
#include <appagg.h>
#include <lazyagg.h>

//
// A dumb "thing that knows about the property manager" class
//


class cPropertyManagerKnower
{
protected:
   static LazyAggMember(IPropertyManager) pPropMan; 
public:

   static IPropertyManager *GetManager() { return pPropMan; };
};


#endif // __PROPKNOW_H

