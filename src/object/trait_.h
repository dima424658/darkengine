/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trait_.h,v 1.5 2000/01/29 13:25:20 adurant Exp $
#pragma once

#ifndef __TRAIT__H
#define __TRAIT__H
#include <appagg.h>
#include <traitbas.h>
#include <trait.h>
#include <trcache.h>

////////////////////////////////////////////////////////////
// 
// BASE TRAIT 
//
// Useful in implementing ITraits 
//
////////////////////////////////////////////////////////////

class cBaseTrait : public ITrait
{
protected:
   sTraitDesc Desc;

public:
   DECLARE_UNAGGREGATABLE();

   cBaseTrait(const sTraitDesc& desc)
      : Desc(desc) {};

   STDMETHOD_(const sTraitDesc*,Describe)() { return &Desc;}; 
   STDMETHOD(Touch)(ObjID ) { return S_OK;};

};

////////////////////////////////////////////////////////////
//
// PREDICATE TRAIT
//
// Abstract class: uses a predicate to determine intrinsicness
//
////////////////////////////////////////////////////////////

class cPredicateTrait : public cBaseTrait
{
   sTraitPredicate Pred;

public:
   cPredicateTrait(const sTraitDesc& desc, const sTraitPredicate& pred)
      : cBaseTrait(desc), Pred(pred) {}
   virtual ~cPredicateTrait() {}; 

   STDMETHOD_(BOOL,IntrinsicTo)(ObjID obj) { return Pred.func(obj,Pred.data); } ;   
  
};

////////////////////////////////////////////////////////////
//
// UNINHERITED TRAIT
// 
// Uses an sTraitPredicate to determine intrinsicness; does no inheritance
// 
////////////////////////////////////////////////////////////

class cUninheritedTrait : public cPredicateTrait
{

public:
   cUninheritedTrait(const sTraitDesc& desc, const sTraitPredicate& pred)
      : cPredicateTrait(desc,pred) {}
   virtual ~cUninheritedTrait() {}; 

   STDMETHOD_(BOOL,PossessedBy)(ObjID obj);
   STDMETHOD_(ObjID, GetDonor)(ObjID obj); 
   STDMETHOD_(IObjectQuery*, GetAllDonors)(ObjID obj);
   STDMETHOD_(IObjectQuery*, GetAllHeirs)(ObjID obj, eObjConcreteness which);

};

////////////////////////////////////////////////////////////
//
// INHERITED TRAIT
// 
// Uses an sTraitPredicate to determine intrinsicness
// 
////////////////////////////////////////////////////////////

class cInheritedTrait : public cPredicateTrait
{
protected:
   ITraitManager* TraitMan; 

public:
   cInheritedTrait(const sTraitDesc& desc, const sTraitPredicate& pred)
      : cPredicateTrait(desc,pred),TraitMan(AppGetObj(ITraitManager)) {}
   virtual ~cInheritedTrait() { SafeRelease(TraitMan);}; 

   STDMETHOD_(BOOL,PossessedBy)(ObjID obj);
   STDMETHOD_(ObjID, GetDonor)(ObjID obj); 
   STDMETHOD_(IObjectQuery*, GetAllDonors)(ObjID obj);
   STDMETHOD_(IObjectQuery*, GetAllHeirs)(ObjID obj, eObjConcreteness which);

};


////////////////////////////////////////////////////////////
//
// CACHED TRAIT
// 
// Uses the IDonorCache to help with searching
// 
////////////////////////////////////////////////////////////

class cCachedTrait : public cInheritedTrait
{
   TraitID ID;

   IDonorCache* pCache;

   BOOL get_cached_donor(ObjID obj, ObjID* donor);

public:
   cCachedTrait(const sTraitDesc& desc, const sTraitPredicate& pred);
   virtual ~cCachedTrait();


   STDMETHOD_(ObjID, GetDonor)(ObjID obj);   
   STDMETHOD_(IObjectQuery*, GetAllDonors)(ObjID obj);   
   STDMETHOD(Touch)(ObjID obj);
};

#endif // __TRAIT__H




