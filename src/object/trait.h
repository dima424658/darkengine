/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trait.h,v 1.3 2000/01/29 13:25:03 adurant Exp $
#pragma once

#ifndef __TRAIT_H
#define __TRAIT_H
#include <osystype.h>
#include <traittyp.h>

////////////////////////////////////////////////////////////
// 
// TRAIT INTERFACE
//
// (see traitman.h for a definition of "trait")
//
//


#undef INTERFACE 
#define INTERFACE ITrait

DECLARE_INTERFACE_(ITrait,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Return my description 
   //
   STDMETHOD_(const sTraitDesc*,Describe)(THIS) PURE;

#define ITrait_Describe(p)    COMCall0(p, Describe)

   //
   // Does an object possess this trait?
   // Does it possess it intrinsically?
   //
   STDMETHOD_(BOOL,PossessedBy)(THIS_ ObjID obj) PURE;
   STDMETHOD_(BOOL,IntrinsicTo)(THIS_ ObjID obj) PURE;

#define ITrait_PossessedBy(p, a)    COMCall2(p, PossessedBy, a)
#define ITrait_IntrinsicTo(p, a)    COMCall2(p, IntrinsicTo, a)


   //
   // Notify the trait that the predicate value 
   // may have changed for an object OBJ_NULL indicates 
   // change for all objects.
   //
   STDMETHOD(Touch)(THIS_ ObjID obj) PURE;

#define ITrait_Touch(p, a) COMCall1(p, Touch, a)

   //
   // Get the nearest donor of the trait to an object.
   // return OBJ_NULL if no such donor exists
   // (note that an object can be its own donor)
   //
   STDMETHOD_(ObjID, GetDonor)(THIS_ ObjID obj) PURE; 

#define ITrait_GetDonor(p, a)    COMCall1(p, GetDonor, a) 
 
   // 
   // Get all the donors of the trait to the object, in order of proximity.
   //
   STDMETHOD_(IObjectQuery*, GetAllDonors)(THIS_ ObjID obj) PURE;
   
   //
   // Get all the objects that inherit the trait from this object
   // filtered by concreteness.
   //
   STDMETHOD_(IObjectQuery*, GetAllHeirs)(THIS_ ObjID obj, eObjConcreteness which) PURE;

#define ITrait_GetAllDonors(p, a)   COMCall1(p, GetAllDonors, a)
#define ITrait_GetAllHeirs(p, a)   COMCall1(p, GetAllHeirs, a)
   
};

#undef INTERFACE

EXTERN ITrait* CreateEmptyTrait(void);

#endif // __TRAIT_H
