/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCINVPR_H
#define __DPCINVPR_H

#ifndef __PROPDEF_H
#include <propdef.h>
#endif // ! __PROPDEF_H

#ifndef RECT_H
#include <rect.h>
#endif // !RECT_H

// Inventory dimensions

// The actual structure
typedef struct sInvDims
{
   int m_width;
   int m_height;
} sInvDims;

// Property stuff

F_DECLARE_INTERFACE(IInvDimsProperty);

#undef INTERFACE
#define INTERFACE IInvDimsProperty

DECLARE_PROPERTY_INTERFACE(IInvDimsProperty)
{
   DECLARE_UNKNOWN_PURE();                  // IUnknown methods 
   DECLARE_PROPERTY_PURE();                 // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sInvDims*);   // Type-specific accessors, by reference
};

#define PROP_INVDIMS "InvDims"

EXTERN IInvDimsProperty *g_InvDimsProperty;

EXTERN void InvDimsPropertyInit();
EXTERN BOOL InvDimsGet(ObjID objID, sInvDims **ppInvDims);

EXTERN int   InvDimsGetWidth(ObjID  objID);
EXTERN int   InvDimsGetHeight(ObjID objID);
EXTERN Point InvDimsGetSize(ObjID   objID);

#define ObjHasInvDims(obj) IProperty_IsRelevant(g_InvDimsProperty,obj)

#define INVDIMS_SET(objID, ppParams) \
   PROPERTY_SET(g_InvDimsProperty, objID, (sInvDims**)ppParams)
#define INVDIMS_DELETE(objID) \
   IProperty_Delete(g_InvDimsProperty, objID)

// Containment dimensions

// The actual structure
typedef struct sContainDims
{
   int m_width;
   int m_height;
} sContainDims;

// Property stuff

F_DECLARE_INTERFACE(IContainDimsProperty);

#undef INTERFACE
#define INTERFACE IContainDimsProperty

DECLARE_PROPERTY_INTERFACE(IContainDimsProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sContainDims*);  // Type-specific accessors, by reference
};

#define PROP_ContainDims "ContainDims"

EXTERN IContainDimsProperty *g_ContainDimsProperty;

EXTERN void ContainDimsPropertyInit();
EXTERN BOOL ContainDimsGet(ObjID objID, sContainDims **ppContainDims);

EXTERN int ContainDimsGetWidth(ObjID objID);
EXTERN int ContainDimsGetHeight(ObjID objID);
EXTERN Point ContainDimsGetSize(ObjID objID);

#define ObjHasContainDims(obj) IProperty_IsRelevant(g_ContainDimsProperty,obj)

#define ContainDims_SET(objID, ppParams) \
   PROPERTY_SET(g_ContainDimsProperty, objID, (sContainDims**)ppParams)
#define ContainDims_DELETE(objID) \
   IProperty_Delete(g_ContainDimsProperty, objID)

#endif // __DPCINVPR_H