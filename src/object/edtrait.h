/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/edtrait.h,v 1.1 1997/12/03 15:43:02 mahk Exp $
#pragma once  
#ifndef __EDTRAIT_H
#define __EDTRAIT_H

#include <edtraity.h>
#include <comtools.h>
#include <objtype.h>

//////////////////////////////////////////////////////////////
// "Editable Trait" interface
//


#undef INTERFACE 
#define INTERFACE IEditTrait
DECLARE_INTERFACE_(IEditTrait,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Return my descriptor
   //
   STDMETHOD_(const sEditTraitDesc*,Describe)(THIS) PURE;

   //
   // Describe my capabilities
   //
   STDMETHOD_(const sEditTraitCaps*,Capabilities)(THIS) PURE;

   //
   // Does an object possess the trait intrinsically (i.e. not through inheritance)
   // Analagous to the ITrait method of the same name.  
   //
   STDMETHOD_(BOOL,IntrinsicTo)(THIS_ ObjID obj) PURE;

   // 
   // Add the trait to an object, causing it to possess the trait intrinsically.
   //
   STDMETHOD(Add)(THIS_ ObjID obj) PURE; 

   //
   // Remove the trait from an object
   // 
   STDMETHOD(Remove)(THIS_ ObjID obj) PURE; 

   //
   // Start a "trait editor" to edit this trait on a particular object
   //
   STDMETHOD(Edit)(THIS_ ObjID obj) PURE; 

   //
   // Parse/Unparse the printed representation of a trait.  
   //
   STDMETHOD(Parse)(THIS_ ObjID obj, const char* val) PURE; 
   STDMETHOD(Unparse)(THIS_ ObjID obj, char* buf, int buflen) PURE; 

}; 



#endif // __EDTRAIT_H
