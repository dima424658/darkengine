/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propman.h,v 1.6 2000/01/29 13:24:38 adurant Exp $
#pragma once

#ifndef __PROPMAN_H
#define __PROPMAN_H
#include <comtools.h>
#include <osystype.h>
#include <proptype.h>

////////////////////////////////////////////////////////////
// PROPERTY MANAGER
//

F_DECLARE_INTERFACE(IPropertyManager); 
F_DECLARE_INTERFACE(IProperty);        

struct sPropertyIter;

#undef INTERFACE
#define INTERFACE IPropertyManager

DECLARE_INTERFACE_(IPropertyManager, IUnknown)
{
   // 
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE(); 
   
   //
   // Look up a property
   // 
   STDMETHOD_(IProperty*,GetProperty)(THIS_ PropertyID) CONSTFUNC PURE;
   STDMETHOD_(IProperty*,GetPropertyNamed)(THIS_ const char* name) CONSTFUNC PURE;

#define IPropertyManager_GetProperty(p, a)         COMCall1(p, GetProperty, a)   
#define IPropertyManager_GetPropertyNamed(p, a)    COMCall1(p, GetPropertyNamed, a)   

   //
   // Notify the property system of an event
   //
   STDMETHOD(Notify)(THIS_ ePropertyNotifyMsg msg, PropNotifyData data) PURE;

#define IPropertyManager_Notify(p, a, b)           COMCall2(p, Notify, a, b)

   //
   // Iterate over all properties
   // 
   STDMETHOD(BeginIter)(THIS_ struct sPropertyIter* iter) PURE;
   STDMETHOD_(IProperty*,NextIter)(THIS_ struct sPropertyIter* iter) PURE;

#define IPropertyManager_BeginIter(p, a)           COMCall1(p, BeginIter, a)
#define IPropertyManager_NextIter(p, a)            COMCall1(p, NextIter, a)

   //
   // Add & remove properties.  These are typically done by the IProperty objects
   // themselves.  Most clients will have no need of these.  
   //

   STDMETHOD_(BOOL,AddProperty)(IProperty *prop, PropertyID *id) PURE;

   // Remove a Property from the list being managed, return success
   STDMETHOD_(BOOL,DelProperty)(IProperty *prop) PURE; 
};

//
// Factory Function 
//


EXTERN tResult LGAPI PropertyManagerCreate(void);

// 
// Property Iter struct
//
typedef struct sPropertyIter
{
   PropertyID id;
} sPropertyIter;


#endif // __PROPMAN_H



