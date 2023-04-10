/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objdef.h,v 1.4 1998/08/20 09:29:27 CCAROLLO Exp $
#pragma once  
#ifndef __OBJDEF_H
#define __OBJDEF_H

#include <objtype.h> 
#include <label.h>

//
// Built-in properties
//
enum eObjSysProperty_
{
   kSymNameProperty,     // symbolic name property
   kTransienceProperty,  // whether we save to disk or not 
   kImmobilityProperty,    // whether we can ever move or not
   
};

//
// Object Concreteness
//

enum eObjConcreteness_
{
   kObjectConcrete = 1,
   kObjectAbstract = 2,
   kObjectAll = (kObjectConcrete|kObjectAbstract)
};

//
// Listener descriptor 
//

struct sObjListenerDesc 
{
   ObjListenerFunc func; 
   void* data; 
};

//
// Partition filter 
//

// return true IFF the obj belongs in our partition 
typedef BOOL (LGAPI * tObjPartitionFilterFunc)(ObjID obj, const struct sObjPartitionFilter* filter); 

struct sObjPartitionFilter
{
   tObjPartitionFilterFunc func; 
   void* data; 
}; 


#endif // __OBJDEF_H






