/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/culpable.h,v 1.1 1998/10/19 13:06:22 mahk Exp $
#pragma once  
#ifndef __CULPABLE_H
#define __CULPABLE_H
#include <objtype.h>
#include <comtools.h>

////////////////////////////////////////////////////////////
// "CULPABILITY" API FOR SIMPLE DAMAGE MODEL
//

//
// The damage model keeps track of who is really "culpable" for damage caused by an 
// object.  Certain objects are marked as capable of culpability using the 
// "culpable" property, and there are certain relationships between objects 
// that cause the "culpability" to be passed from object to object.  
//

enum eCulpFlags
{
   kCulpTransitive = 1 << 0, // propagate even if "from" is not immediately culpable
};

//
// If "from" is a culpable object, make it culpable for "to". 
// If the "transitive" bit is set, then make whoever is culpable for
// "from" also be culpable for "to".  
//

EXTERN void PropagateCulpability(ObjID from, ObjID to, ulong flags);  

//
// Track and object back to the object culpable for it
//
EXTERN ObjID GetRealCulprit(ObjID culprit); 

//
// Declare a link relation to be "culpability" relation.  Culpability
// will propagate whenever links are formed, using the specified
// flags.  
//
F_DECLARE_INTERFACE(IRelation); 
EXTERN void AddCulpabilityRelation(IRelation* rel, ulong flags); 


#endif  // __CULPABLE_H
