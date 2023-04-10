/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/posprop.h,v 1.2 1998/03/26 13:51:51 mahk Exp $
#pragma once  
#ifndef __POSPROP_H
#define __POSPROP_H

#include <property.h>

////////////////////////////////////////////////////////////
// POSITION PROPERTY INTERFACE AND NAME
//

typedef struct Position Position; 

#undef INTERFACE
#define INTERFACE IPositionProperty 
DECLARE_PROPERTY_INTERFACE(IPositionProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(Position*); 
}; 


// The name of the position prop
#define PROP_POSITION_NAME "Position"

#endif // __POSPROP_H




