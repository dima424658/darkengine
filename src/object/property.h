/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/property.h,v 1.15 2000/01/29 13:24:22 adurant Exp $
#pragma once

#ifndef PROPERTY__H
#define PROPERTY__H

#include <comtools.h>
#include <objtype.h>
#include <proptype.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// PROPERTY SYSTEM COM INTERFACES
//
// NOTE: Make sure that any change to this interface is also made in propdef.h

F_DECLARE_INTERFACE(IProperty);
#undef INTERFACE
#define INTERFACE IProperty

DECLARE_INTERFACE_ (IProperty, IUnknown)
{
   // IUnknown methods
   DECLARE_UNKNOWN_PURE ();

//
//  Return my description 
// 
STDMETHOD_ (const sPropertyDesc *, Describe) (THIS) CONSTFUNC PURE; 

//
//  Return my ID 
// 
STDMETHOD_(PropertyID, GetID) (THIS) CONSTFUNC PURE; 

//
//  Return a description of my type 
// 
STDMETHOD_ (const sPropertyTypeDesc *, DescribeType) (THIS) CONSTFUNC PURE; 

//
//  Force this property to be relevent for the object, use a default value, return success 
// 
STDMETHOD(Create)(THIS_ ObjID obj) PURE; 

//
//  Set the property for an object, given another object as an example to copy 
// 
STDMETHOD(Copy)(THIS_ ObjID to, ObjID from) PURE; 

//
//  Decide that this property is irrelevant for the object 
// 
STDMETHOD(Delete) (THIS_ ObjID obj) PURE; 

//
//  Return whether an object possesses a property 
// 
STDMETHOD_(BOOL, IsRelevant) (THIS_ ObjID obj) CONSTFUNC PURE; 

//
// Return whether an object posesses the property *without inheritance*  
// 

STDMETHOD_(BOOL, IsSimplyRelevant) (THIS_ ObjID obj) CONSTFUNC PURE; 

//
//  Refresh the property as if it had just been set for a particular obj 
// 
STDMETHOD_(BOOL,Touch) (THIS_ ObjID obj) PURE; 

//
// Notify the property of various events. Used by the property manager mainly.  
// 
STDMETHOD_(void, Notify) (THIS_ ePropertyNotifyMsg msg, PropNotifyData data) PURE; 

//
//  Listen for property changes to an object 
// 
STDMETHOD_(PropListenerHandle, Listen) (THIS_ PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data) PURE; 

STDMETHOD(Unlisten)(THIS_ PropListenerHandle listener) PURE; 
//
//  Start iterating over all objects with this property 
// 

STDMETHOD_(void, IterStart) (THIS_ sPropertyObjIter* iter) CONSTFUNC PURE; 

//
//  Fill in obj with the next obj  in iteration, return FALSE= done.
// 
STDMETHOD_(BOOL, IterNext) (THIS_ sPropertyObjIter* iter,ObjID* next) CONSTFUNC PURE; 

//
//  Clean up after iteration 
// 
STDMETHOD_(void, IterStop) (THIS_ sPropertyObjIter* iter) CONSTFUNC PURE;  
};

////////////////////////////////////////

#define IProperty_Describe(p)             COMCall0(p, Describe)
#define IProperty_GetID(p)                COMCall0(p, GetID)
#define IProperty_DescribeType(p)         COMCall0(p, DescribeType)
#define IProperty_Create(p,a)             COMCall1(p, Create, a)
#define IProperty_Copy(p,a,b)             COMCall2(p, Copy, a, b)
#define IProperty_Delete(p,a)             COMCall1(p, Delete, a)
#define IProperty_IsRelevant(p,a)         COMCall1(p, IsRelevant, a)
#define IProperty_IsSimplyRelevant(p,a)   COMCall1(p, IsSimplyRelevant, a)
#define IProperty_Touch(p,a)              COMCall1(p, Touch, a)
#define IProperty_Notify(p, a, b)         COMCall2(p, Notify, a, b)
#define IProperty_Listen(p, a, b, c)      COMCall3(p, Listen, a, b, c)
#define IProperty_IterStart(p, a)         COMCall1(p, IterStart, a)
#define IProperty_IterNext(p, a, b)       COMCall2(p, IterNext, a, b)
#define IProperty_IterStop(p, a)          COMCall1(p, IterStop, a)


////////////////////////////////////////////////////////////
// PROPERTY ACCESSORS
//

#define PROPERTY_GET(p, a, b)          COMCall2(p, Get, a, b)
#define PROPERTY_GET_SIMPLE(p, a, b)   COMCall2(p, GetSimple, a, b)
#define PROPERTY_SET(p, a, b)          COMCall2(p, Set, a, b)
#define PROPERTY_ITER_NEXT_VALUE(p, a, b, c)    COMCall3(p, IterNextValue, a, b,c)

#ifdef __cplusplus
class cPropertyImpl;
#else
typedef void cPropertyImpl;
#endif 

//
// These are for use by macros only.  They violate COM refcount conventions.
// REPEAT: DO NOT CALL THESE DIRECTLY
//
EXTERN IProperty* _GetProperty(PropertyID id);
EXTERN IProperty* _GetPropertyNamed(const char* name); 

////////////////////////////////////////////////////////////
// LOOK UP A PROPERTY BY ID OR NAME
//

EXTERN IProperty* GetProperty(PropertyID id);
EXTERN IProperty* GetPropertyNamed(const char* name); 

//
// Interfaces 
//

#define GetPropertyInterface(id,IFACE,ppFace) \
        (COMQueryInterface(_GetProperty(id),IID_##IFACE,(void**)ppFace) == S_OK)
#define GetPropertyInterfaceNamed(name,IFACE,ppFace) \
        (COMQueryInterface(_GetPropertyNamed(name),IID_##IFACE,(void**)ppFace) == S_OK)





#endif // PROPERTY__H


/*
Local Variables:
Typedefs:("BOOL" "ObjID" "REFIID" "ePropertyImpl")
End:
*/




