/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitman.h,v 1.12 2000/01/29 13:25:09 adurant Exp $
#pragma once

#ifndef __TRAITMAN_H
#define __TRAITMAN_H
#include <comtools.h>
#include <traittyp.h>

////////////////////////////////////////////////////////////
// 
// THE TRAIT MANAGER.
//
// A "trait" is any fact about an object (i.e. a boolean value)
// that can be inherited from its "object type."
//
// The trait manager expresses an object's type as a special 
// first-class object called an "archetype."  An object 
// possesses all the traits that its archetype has.  
// Since archetypes are first-class objects, they too 
// have archetypes, and thus the object system is a 
// hierarchy.
//
// In addition to its single archetype, an object can have
// any number of _metaproperties_, first-class objecst from 
// which it can also inherit traits.  Archetypes may 
// have metaproperties.  Metaproperties and archetypes have 
// priorities for the purpose of determining *which* archetype or
// metaproperty is the *first* donor of a trait to 
// an object. 
//
// Unlike Archetypes, metaproperties can be _suppressed_
// by objects.  An object that suppresses a metaproperty 
// will not inherit from that metaproperty, even if its 
// archetypes and metaproperties do.  Suppression occurs
// at a particular priority level, which higher-level
// metaproperties can ignore.  
//
// Occaisionally in comments, C++ jargon will be used to describe 
// the hierarchy of archetypes.  The word "class" will not be 
// used to mean archetype.  The archetypes and metaproperties that 
// contribute a trait to an object are its "donors."  An object is never 
// its own donor.
//
// 

#undef INTERFACE 
#define INTERFACE ITraitManager

DECLARE_INTERFACE_(ITraitManager,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //------------------------------------------------------------
   // Archetypes

   //
   // Creates a "base archetype."  Base archetypes always derive from ROOT_ARCHETYPE
   // If there is already an archetype with the specified name, S_EXISTS is returned
   // and the already-existing archetype is used.
   //
   // The ObjID of the created archetype is returned. 
   // 
   STDMETHOD(CreateBaseArchetype)(THIS_ const char* name, ObjID* pArchetypeID) PURE;
   
#define ITraitManager_CreateBaseArchetype(p,a,b)   COMCall2(p, CreateBaseArchetype, a, b)

   //
   // Create a new archetype, specifying "parent" as its archetype
   //
   STDMETHOD_(ObjID,CreateArchetype)(THIS_ const char* name, ObjID parent) PURE;
   STDMETHOD_(ObjID,CreateConcreteArchetype)(THIS_ const char* name, ObjID parent) PURE;

#define ITraitManager_CreateArchetype(p, a, b)          COMCall2(p, CreateArchetype, a, b)
#define ITraitManager_CreateConcreteArchetype(p, a, b)  COMCall2(p, CreateConcreteArchetype, a, b)

   //
   // Archetype Manipulation
   //
   STDMETHOD_(ObjID, GetArchetype)(THIS_ ObjID obj) PURE;
   STDMETHOD(SetArchetype)(THIS_ ObjID obj, ObjID arch) PURE;
   STDMETHOD_(BOOL, IsArchetype)(THIS_ ObjID obj) PURE;

#define ITraitManager_GetArchetype(p, a)  COMCall1(p, GetArchetype, a)
#define ITraitManager_SetArchetype(p, a, b)  COMCall3(p, SetArchetype, a, b)
#define ITraitManager_IsArchetype(p, a)   COMCall1(p, IsArchetype a)


   //------------------------------------------------------------
   // METAPROPERTIES
   //
   
   //
   // Get the root metaproperty
   //
   STDMETHOD_(ObjID,RootMetaProperty)(THIS) PURE;

#define ITraitManager_RootMetaProperty(p)    COMCall0(p, RootMetaProperty)

   //
   // Create a new metaproperty, specifying "parent" as its archetype
   //
   STDMETHOD_(ObjID,CreateMetaProperty)(THIS_ const char* name, ObjID parent) PURE; 

#define ITraitManager_CreateMetaProperty(p, a) COMCall1(p, CreateMetaProperty, a)

   //
   // Detect metaproperties
   //
   STDMETHOD_(BOOL,IsMetaProperty)(THIS_ ObjID obj) PURE;

#define ITraitManager_IsMetaProperty(p, a)   COMCall2(p, IsMetaProperty, a)


   //
   // Add a MetaProperty to an object
   //
   STDMETHOD(AddObjMetaProperty)(THIS_ ObjID obj, ObjID meta) PURE;
   // with priority
   STDMETHOD(AddObjMetaPropertyPrioritized)(THIS_ ObjID obj, ObjID meta, tMetaPropertyPriority) PURE;

#define ITraitManager_AddObjMetaProperty(p, a, b) COMCall3(p, AddObjMetaProperty, a, b)
#define ITraitManager_AddObjMetaPropertyPrioritized(p, a, b) COMCall3(p, AddObjMetaPropertyPrioritized, a, b)

   //
   // Remove a metaproperty from an object
   //             
   STDMETHOD(RemoveObjMetaProperty)(THIS_ ObjID obj, ObjID meta) PURE;
   // with priority
   STDMETHOD(RemoveObjMetaPropertyPrioritized)(THIS_ ObjID obj, ObjID meta, tMetaPropertyPriority) PURE;

#define ITraitManager_RemoveObjMetaProperty(p, a, b) COMCall3(p, RemoveObjMetaProperty, a, b)
#define ITraitManager_RemoveObjMetaPropertyPrioritized(p, a, b) COMCall3(p, RemoveObjMetaPropertyPrioritized, a, b)

   //------------------------------------------------------------
   // QUERIES AND TRAITS
   //

   //
   // Check whether an object possesses a metaproperty or archetype
   //

   // inherited
   STDMETHOD_(BOOL,ObjHasDonor)(THIS_ ObjID obj, ObjID donor) PURE;
   // intrinsically...
   STDMETHOD_(BOOL,ObjHasDonorIntrinsically)(THIS_ ObjID obj, ObjID donor) PURE;

#define ITraitManager_ObjHasDonor(p, a, b) COMCall2(p, ObjHasDonor, a, b)
#define ITraitManager_ObjHasDonorIntrinsically(p, a, b) COMCall2(p, ObjInheritsMetaProperty, a, b)

   //
   // Check whether an object is valid for inheritance
   //
   STDMETHOD_(BOOL,ObjIsDonor)(THIS_ ObjID donor) PURE;

#define ITraitManager_ObjIsDonor(p, a) COMCall1(p, ObjIsDonor, a)

   //
   // Run simple traitless queries on the hierarchy. 
   //
   STDMETHOD_(IObjectQuery*, Query)(THIS_ ObjID obj, eTraitQueryType type) PURE;

#define ITraitManager_Query(p, a, b) COMCall2(p, Query, a, b)

   //
   // Create a new trait, from a simple predicate function
   //
   STDMETHOD_(ITrait*,CreateTrait)(THIS_ const sTraitDesc* desc, const sTraitPredicate* pred) PURE;

#define ITraitManager_CreateTrait(p, a) COMCall1(p, CreateTrait, a)

   //
   // Listen to changes in the hierarcy 
   //
   STDMETHOD(Listen)(THIS_ HierarchyListenFunc func, HierarchyListenerData data) PURE;

   //------------------------------------------------------------
   // REGISTRATION AND NOTIFICATION
   //

   //
   // Add an object, specifying its archetype.  Fails if 
   // the object has already been added.  
   //
   STDMETHOD(AddObject)(THIS_ ObjID obj, ObjID archetype) PURE;
   // Remove an object
   STDMETHOD(RemoveObject)(THIS_ ObjID obj) PURE; 

#define ITraitManager_AddObject(p, a, b)     COMCall3(p, AddObject, a, b)
#define ITraitManager_RemoveObject(p, a, b)  COMCall3(p, RemoveObject, a, b)


   //
   // Notify the trait manager of object changes
   //
   STDMETHOD(Notify)(THIS_ eObjNotifyMsg msg, ObjNotifyData data) PURE;

      
};


#undef INTERFACE

EXTERN tResult LGAPI TraitManagerCreate(void);


#endif // __TRAITMAN_H
