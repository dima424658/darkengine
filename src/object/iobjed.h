/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/iobjed.h,v 1.4 1997/12/04 15:03:49 mahk Exp $
#pragma once  
#ifndef __IOBJED_H
#define __IOBJED_H

#include <comtools.h>
#include <objtype.h>
#include <objedtyp.h>

//
// Object Editor Interface
//

#undef INTERFACE
#define INTERFACE IObjEditor
DECLARE_INTERFACE_(IObjEditor,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Start the editor.  
   // If Modal, returns TRUE iff the ok button was pushed. 
   // If Modeless, returns FALSE immediately
   //
   STDMETHOD_(BOOL,Go)(THIS_ eObjEdModality modal) PURE;

#define IObjEditor_Go(p, a) COMCall1(p, Go, a) 

   //
   // Set the callback to be called when buttons get pushed
   //
   STDMETHOD(SetCallback)(THIS_ ObjEditCB cb, ObjEditCBData data) PURE;

#define IObjEditor_SetCallback(p, a, b) COMCall2(p, SetCallback, a, b)

   //
   // Accessors
   //
   STDMETHOD_(const sObjEditorDesc*,Describe)(THIS) PURE;
   STDMETHOD_(ObjID,Object)(THIS) PURE; 

#define IObjEditor_Describe(p)         COMCall0(p, Describe)
#define IObjEditor_Object(p)           COMCall0(p, Object)

};

#undef INTERFACE 

// 
// Object Editing Service Agg Member
//

F_DECLARE_INTERFACE(IEditTrait); 
F_DECLARE_INTERFACE(IProperty);

#define INTERFACE IObjEditors
DECLARE_INTERFACE_(IObjEditors,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Registry of "traits" that can be edited
   //
   STDMETHOD(AddTrait)(THIS_ IEditTrait* trait) PURE; 
   STDMETHOD(RemoveTrait)(THIS_ IEditTrait* trait) PURE; 

   // Make a trait out of a property, and add it. 
   STDMETHOD_(IEditTrait*, AddProperty)(THIS_ IProperty* prop) PURE; 
   
   // Iteration
   STDMETHOD_(IEditTrait*,FirstTrait)(THIS_ sEditTraitIter* iterstate) PURE;
   STDMETHOD_(IEditTrait*,NextTrait)(THIS_ sEditTraitIter* iterstate) PURE; 

   //
   // Create an object editor
   //
   STDMETHOD_(IObjEditor*,Create)(THIS_ sObjEditorDesc* desc, ObjID obj) PURE;

#define IObjEditors_Create(p, a, b)       COMCall2(p, Create, a, b) 
}; 

#undef INTERFACE 

EXTERN void ObjEditorsCreate(void); 

#endif // __IOBJED_H




