/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitnul.h,v 1.2 2000/01/29 13:25:12 adurant Exp $
#pragma once
#ifndef __TRAITNUL_H
#define __TRAITNUL_H
#include <trait_.h>

////////////////////////////////////////////////////////////
// THE NULL TRAIT
// 
// This class implements the trait that is not intrinsic to 
// any object
//

class cNullTrait : public cBaseTrait 
{
public:
   cNullTrait();
   virtual ~cNullTrait(); 

   STDMETHOD_(BOOL,PossessedBy)(ObjID obj);
   STDMETHOD_(BOOL,IntrinsicTo)(ObjID obj);
   STDMETHOD_(ObjID, GetDonor)(ObjID obj); 
   STDMETHOD_(IObjectQuery*, GetAllDonors)(ObjID obj);
   STDMETHOD_(IObjectQuery*, GetAllHeirs)(ObjID obj, eObjConcreteness which);
}; 


#endif // __TRAITNUL_H
