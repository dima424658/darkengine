/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/autoprop.h,v 1.2 1998/03/26 13:51:33 mahk Exp $
#pragma once  
#ifndef __AUTOPROP_H
#define __AUTOPROP_H

///////////////////////////////////////////////////////////////////////////////
//
// AUTOMAP PROPERTY 
//
#include <property.h>

#define PROP_AUTOMAP_NAME "Automap"

typedef struct sAutoMapProperty
{
   ulong page;       // Page Number
   ulong location;   // Location number
} sAutomapProperty;

F_DECLARE_INTERFACE(IAutomapProperty);

#undef  INTERFACE
#define INTERFACE IAutomapProperty;
DECLARE_PROPERTY_INTERFACE(IAutomapProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sAutomapProperty*); 
}; 
#undef INTERFACE

EXTERN void AutomapPropInit(void);
EXTERN void AutomapPropTerm(void);

#endif // __AUTOPROP_H


