/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/flowarch.h,v 1.1 1998/08/11 14:12:50 mahk Exp $
#pragma once  
#ifndef __FLOWARCH_H
#define __FLOWARCH_H

#include <property.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// FLOW ARCHETYPE SUPPORT
//
// Each flow group has a "flow archetype," that is created on demand.  
//
// The "flow index" is property that maps the flow archetypes 
// into flow group indices.  
// It has an "inverse accessor," capable of finding (or creating) the
// archetype for a particular flow group.    
// 

//------------------------------------------------------------
// PROPERTY INTERFACE
//

#undef INTERFACE
#define INTERFACE IFlowGroupProperty
DECLARE_PROPERTY_INTERFACE(IFlowGroupProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 

   // property accessors, without set
   DECLARE_PROPERTY_READ_ACCESSORS(int);

   //
   // Inverse accessor.  Get an obj from an index.  
   // Note that this will create an object if none exists. 
   //
   STDMETHOD_(ObjID, GetObj)(THIS_ int idx) PURE; 

}; 

#endif  // __FLOWARCH_H

#define PROP_FLOW_GROUP_NAME "FlowGroup"

//
// Init/Term
//
EXTERN void FlowGroupPropInit(void); 
EXTERN void FlowGroupPropTerm(void); 





