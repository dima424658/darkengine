/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/iobjsys.h,v 1.16 2000/02/24 23:40:58 mahk Exp $
#pragma once
#ifndef __IOBJSYS_H
#define __IOBJSYS_H

#include <objtype.h>
#include <comtools.h>

F_DECLARE_INTERFACE(IObjectQuery);
F_DECLARE_INTERFACE(IObjectSystem);
F_DECLARE_INTERFACE(IProperty);

#undef INTERFACE
#define INTERFACE IObjectSystem
DECLARE_INTERFACE_(IObjectSystem,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Simple Object Creation 
   // If the "archetype" is not actually an archetype, then creates a clone
   // of the "archetype" object using CloneObject
   //
   STDMETHOD_(ObjID,Create)(THIS_ ObjID archetype, eObjConcreteness concrete) PURE;

   // 
   // Complex object creation
   //
   // This is used when you need to create an object and set up its properties before 
   // it is seen by the rest of the world.  This is done by starting an 
   // "object creation transaction," adding properties to the created object, and then
   // ending the "object creation transaction."
   //
   STDMETHOD_(ObjID,BeginCreate)(THIS_ ObjID archetype, eObjConcreteness concrete) PURE;
   STDMETHOD(EndCreate)(THIS_ ObjID obj) PURE;

   //
   // Destroy an object 
   // 
   STDMETHOD(Destroy)(THIS_ ObjID obj) PURE;

   //
   // Copy an object's properties (and other clonable traits)
   // Does not actually create a new object.
   // 
   STDMETHOD(CloneObject)(THIS_ ObjID to, ObjID from) PURE;

   //
   // Reset the entire database
   //
   STDMETHOD(Reset)(THIS) PURE;

   //
   // Return whether an object exists
   //
   STDMETHOD_(BOOL,Exists)(THIS_ ObjID obj) PURE;

   // 
   // Object Naming
   //
   STDMETHOD(NameObject)(THIS_ ObjID obj, const char* name) PURE;
   STDMETHOD_(const char*,GetName)(THIS_ ObjID obj) PURE;
   STDMETHOD_(ObjID,GetObjectNamed)(THIS_ const char* name) PURE;

   //
   // Transience, i.e. whether we are saved to disk
   //
   STDMETHOD(SetObjTransience)(THIS_ ObjID obj, BOOL transient) PURE;
   STDMETHOD_(BOOL,ObjIsTransient)(THIS_ ObjID obj) PURE;

   //
   // Immobility, i.e. whether we can ever be moved
   //
   STDMETHOD(SetObjImmobility)(THIS_ ObjID obj, BOOL immobile) PURE;
   STDMETHOD_(BOOL,ObjIsImmobile)(THIS_ ObjID obj) PURE;

   //
   // Iteration
   //
   STDMETHOD_(IObjectQuery*,Iter)(THIS_ eObjConcreteness which) PURE;

   //
   // Built-in properties
   //
   STDMETHOD_(IProperty*,GetCoreProperty)(THIS_ eObjSysProperty which) PURE;

   //
   // Tell the object system to perform a major database change 
   //
   STDMETHOD(DatabaseNotify)(THIS_ eObjNotifyMsg msg, ObjNotifyData data) PURE;

   //
   // Cause Destroyed objects to be actually removed
   // *To Be Removed* 
   //
   STDMETHOD(PurgeObjects)(THIS) PURE;

   //
   // Listen for changes to objects
   //
   STDMETHOD_(tObjListenerHandle,Listen)(THIS_ sObjListenerDesc* desc) PURE; 
   STDMETHOD(Unlisten)(THIS_ tObjListenerHandle handle) PURE; 

   //
   // Size of the object space
   //
   STDMETHOD_(ObjID,MaxObjID)(THIS) PURE;
   STDMETHOD_(ObjID,MinObjID)(THIS) PURE;
   STDMETHOD_(int,ActiveObjects)(THIS) PURE;

   // 
   // "lock" the object system so that no objects will be destroyed.
   // 
   STDMETHOD(Lock)(THIS) PURE; 

   //
   // Unlock the object system, destroying any objects that Destroy()
   // had been called on while the system was locked
   // 
   STDMETHOD(Unlock)(THIS) PURE;
   
   ////////////////////////////////////////////////////////////
   //
   // Object Space Partitioning/Filtering/Remapping for save/load
   // 
   // See objnotif.h for the object partitions set.
   //

   //
   // Test whether an object belongs to a partition for the purposes of saving it.
   // before you save an object, make sure that it belongs to the partition you're saving. 
   //
   STDMETHOD_(BOOL,IsObjSavePartition)(THIS_ ObjID obj, eObjPartition part) PURE; 

   //
   // Test whether an object belongs to a partition for the purposes of loading it. 
   // before you load an object, make sure that it belongs to the partition you're loading. 
   //
   STDMETHOD_(BOOL,IsObjLoadPartition)(THIS_ ObjID obj, eObjPartition part) PURE; 

   //
   // Get the smallest partition that an object will save in.  
   // 
   STDMETHOD_(eObjPartition,ObjDefaultPartition)(THIS_ ObjID obj) PURE; 

   //
   // Remap an ObjID on load.  When you load an object, call this to get it's real ObjID. 
   //
   STDMETHOD_(ObjID,RemapOnLoad)(THIS_ ObjID src) PURE;

   //
   // Set the filter that describes which objects are part of a particular sub-partition
   //
   STDMETHOD(SetSubPartitionFilter)(THIS_ eObjPartition subpart, const sObjPartitionFilter* filter) PURE; 

};

#undef INTERFACE 

#define IObjectSystem_Create(p, a, b)        COMCall2(p, Create, a, b) 
#define IObjectSystem_BeginCreate(p, a, b)   COMCall2(p, BeginCreate, a, b)
#define IObjectSystem_EndCreate(p, a)        COMCall1(p, EndCreate, a)
#define IObjectSystem_Destroy(p, a)          COMCall1(p, Destroy, a)
#define IObjectSystem_Reset(p)               COMCall0(p, Reset)
#define IObjectSystem_Exists(p, a)           COMCall1(p, Exists, a)
#define IObjectSystem_Iter(p, a)             COMCall1(p, Iter, a)
#define IObjectSystem_DatabaseNotify(p, a, b)COMCall2(p, DatabaseNotify, a, b)
#define IObjectSystem_NameObject(p, a, b)    COMCall2(p, NameObject, a, b)
#define IObjectSystem_GetName(p, a)          COMCall1(p, GetName, a)
#define IObjectSystem_GetObjectNamed(p, a)   COMCall1(p, GetObjectNamed, a)
#define IObjectSystem_GetCoreProperty(p, a)  COMCall1(p, GetCoreProperty, a)
#define IObjectSystem_SetObjTransience(p, a, b) COMCall2(p, SetObjTransience, a, b)
#define IObjectSystem_ObjIsTransient(p, a)   COMCall1(p, ObjIsTransient, a)
#define IObjectSystem_SetObjImmobility(p, a, b) COMCall2(p, SetObjImmobility, a, b)
#define IObjectSystem_ObjIsImmobile(p, a)    COMCall1(p, ObjIsImmobile, a)
#define IObjectSystem_PurgeObjects(p)        COMCall0(p, PurgeObjects)
#define IObjectSystem_Listen(p, a)           COMCall1(p, Listen, a)
#define IObjectSystem_Unlisten(p, a)         COMCall1(p, Unlisten, a)
#define IObjectSystem_SetSubPartitionFilter(p,a,b)    COMCall2(p, SetSubPartitionFilter, a, b)
#define IObjectSystem_Lock(p)                COMCall0(p, Lock)
#define IObjectSystem_Unlock(p)              COMCall0(p, Unlock)


EXTERN tResult LGAPI ObjectSystemCreate(void);



#endif // __IOBJSYS_H
