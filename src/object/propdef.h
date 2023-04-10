/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propdef.h,v 1.5 1998/10/08 16:30:10 MROWLEY Exp $
#pragma once  
#ifndef __PROPDEF_H
#define __PROPDEF_H

#include <comtools.h>
#include <objtype.h>
#include <proptype.h>
#include <property.h>


//
// PROPERTY INTERFACE MACROS
//

// 
// A Property interface declaration looks like this:
// #undef INTERFACE   
// #define INTERFACE IFooProperty         // standard COM thing
// DECLARE_PROPERTY_INTERFACE(IFooProperty)
// { 
//    DECLARE_UNKNOWN_PURE();  // IUnknown methods
//    DECLARE_PROPERTY_PURE();      // IProperty methods 
//    DECLARE_PROPERTY_ACCESSORS(Foo);  // Accessors for type Foo, (to set/get by reference, use Foo* instead)
// }; 

#define DECLARE_PROPERTY_INTERFACE(iface) \
   F_DECLARE_INTERFACE(iface); \
   DECLARE_INTERFACE_(iface,IProperty)

#define DECLARE_PROPERTY_READ_ACCESSORS(TYPE) \
   STDMETHOD_ (BOOL, Get) (THIS_ ObjID obj, TYPE (*ptr)) CONSTFUNC PURE; \
   STDMETHOD_ (BOOL, GetSimple) (THIS_ ObjID obj, TYPE (*ptr)) CONSTFUNC PURE; \
   STDMETHOD_ (BOOL, IterNextValue) (THIS_ sPropertyObjIter* iter,ObjID* next, TYPE (*val)) CONSTFUNC PURE \


#define DECLARE_PROPERTY_ACCESSORS(TYPE) \
   STDMETHOD(Set) (THIS_ ObjID obj, TYPE val) PURE; \
   STDMETHOD_(BOOL,TouchValue)(THIS_ ObjID obj, TYPE val) PURE; \
   DECLARE_PROPERTY_READ_ACCESSORS(TYPE)

//
// DECLARE_PROPERTY()
//
// This macro is used for generating COM interfaces for properties.  
// It *MUST* be kept in sync with the IProperty interface

#define DECLARE_PROPERTY_PURE() \
/* Return my description */ \
STDMETHOD_ (const sPropertyDesc *, Describe) (THIS) CONSTFUNC PURE; \
/* Return my ID */ \
STDMETHOD_(PropertyID, GetID) (THIS) CONSTFUNC PURE; \
/* Return a description of my type */ \
STDMETHOD_ (const sPropertyTypeDesc *, DescribeType) (THIS) CONSTFUNC PURE; \
/* Force this property to be relevent for the object, use a default value, return success */ \
STDMETHOD(Create)(THIS_ ObjID obj) PURE; \
/* Set the property for an object, given another object as an example to copy */ \
STDMETHOD(Copy)(THIS_ ObjID to, ObjID from) PURE; \
/* Decide that this property is irrelevant for the object */ \
STDMETHOD(Delete) (THIS_ ObjID obj) PURE; \
/* Return whether an object possesses a property */ \
STDMETHOD_(BOOL, IsRelevant) (THIS_ ObjID obj) CONSTFUNC PURE; \
/* Return whether an object posesses the property *without inheritance*  */ \
STDMETHOD_(BOOL, IsSimplyRelevant) (THIS_ ObjID obj) CONSTFUNC PURE; \
/* Refresh the property as if it had just been set for a particular obj */ \
STDMETHOD_(BOOL,Touch) (THIS_ ObjID obj) PURE; \
/*/ Notify the property system of various events */ \
STDMETHOD_(void, Notify) (THIS_ ePropertyNotifyMsg msg, PropNotifyData data) PURE; \
/* Listen for property changes to an object */ \
STDMETHOD_(PropListenerHandle, Listen) (THIS_ PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data) PURE; \
/* Stop listening */ \
STDMETHOD(Unlisten) (THIS_ PropListenerHandle handle) PURE; \
/* Start iterating over all objects with this property */ \
STDMETHOD_(void, IterStart) (THIS_ sPropertyObjIter* iter) CONSTFUNC PURE; \
/* Fill in obj with the next obj  in iteration, return FALSE= done.*/ \
STDMETHOD_(BOOL, IterNext) (THIS_ sPropertyObjIter* iter,ObjID* next) CONSTFUNC PURE; \
/* Clean up after iteration */ \
STDMETHOD_(void, IterStop) (THIS_ sPropertyObjIter* iter) CONSTFUNC PURE

#endif // __PROPDEF_H
