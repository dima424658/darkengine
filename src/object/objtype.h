/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objtype.h,v 1.6 2000/01/29 13:24:08 adurant Exp $
#pragma once

#ifndef _OBJTYPE_H
#define _OBJTYPE_H

#include <lg.h>

//
// Object IDs
//
typedef  int   ObjID;
#define OBJ_NULL 0

// The root of all archetypes
#define ROOT_ARCHETYPE (-1)

// Abstract vs concrete objects 
#define OBJ_IS_ABSTRACT(obj)   ((ObjID)(obj) < (ObjID)0)
#define OBJ_IS_CONCRETE(obj)   ((ObjID)(obj) > (ObjID)0)

// 
// Other types used by IObjectSystem 
//

// bitmask of whether an object is concrete or abstract 
typedef ulong eObjConcreteness; 

// notification messages
typedef ulong eObjNotifyMsg;
typedef void* ObjNotifyData;

// listener descriptor
typedef struct sObjListenerDesc sObjListenerDesc; 
typedef void (* ObjListenerFunc)(ObjID obj, eObjNotifyMsg msg, void* data); 
typedef int tObjListenerHandle; 

typedef struct Label Label;

// enum for selecting a property implemented by the objsys
typedef ulong eObjSysProperty;


// Every object belongs to a "partition."  A save file contains one or more object partitions. 
typedef ulong eObjPartition; 

typedef struct sObjPartitionFilter sObjPartitionFilter; 


#endif // _OBJTYPE_H 


