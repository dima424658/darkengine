/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/edittool.h,v 1.2 1997/10/16 18:19:00 mahk Exp $
#pragma once  
#ifndef __EDITTOOL_H
#define __EDITTOOL_H

#include <comtools.h>
#include <objtype.h>

////////////////////////////////////////////////////////////
// EDITOR TOOLS COMPONENT
//
// This aggregate member provides useful editing interfaces to DLLs that want them. 
//

F_DECLARE_INTERFACE(IEditTools);

#undef INTERFACE
#define INTERFACE IEditTools

DECLARE_INTERFACE_(IEditTools,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Get a human-readable name for an object 
   //
   STDMETHOD_(const char*,ObjName)(THIS_ ObjID obj) PURE;

   // 
   // Parse an object by name
   //
   STDMETHOD_(ObjID,GetObjNamed)(THIS_ const char* name) PURE; 

   //
   // Edit an object with a registered type name
   // 
   STDMETHOD(EditTypedData)(THIS_ const char* title, const char* type, void* data) PURE;

};

#undef INTERFACE

//
// Factory 
//

EXTERN void EditToolsCreate(void); 




#endif // __EDITTOOL_H
