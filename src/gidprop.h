/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/gidprop.h,v 1.4 2000/01/29 12:41:18 adurant Exp $
#pragma once

#ifndef GIDPROP_H
#define GIDPROP_H
#include <property.h>

typedef struct GlobalID GlobalID;

//------------------------------------------------------------
// GID PROPERTY INTERFACE 
//

#undef INTERFACE
#define INTERFACE IGlobalIDProperty
DECLARE_PROPERTY_INTERFACE(IGlobalIDProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(GlobalID*); 
}; 
#undef INTERFACE


//------------------------------------------------------------
// GID PROPERTY NAME
//

#define PROP_GID_NAME "GID"

//------------------------------------------------------------
// GID PROPERTY API
//



EXTERN void GlobalIDPropInit(void);



#endif // GIDPROP_H
