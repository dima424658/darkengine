/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/brethprp.h,v 1.1 1998/06/22 23:26:13 mahk Exp $
#pragma once  
#ifndef __BRETHPRP_H
#define __BRETHPRP_H

#include <property.h>
#include <propdef.h>

struct sBreathConfig; 

#undef INTERFACE 
#define INTERFACE IBreathProperty
DECLARE_PROPERTY_INTERFACE(IBreathProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(struct sBreathConfig*); 
}; 

#undef INTERFACE 



#endif // __BRETHPRP_H
