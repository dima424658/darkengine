/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trcache_.h,v 1.6 2000/01/29 13:25:23 adurant Exp $
#pragma once

#ifndef __TRCACHE__H
#define __TRCACHE__H

#include <aggmemb.h>
#include <trcache.h>

#include <hashset.h>
#include <hashpp.h>
#include <dlist.h>
#include <linktype.h>
#include <dynarray.h>


////////////////////////////////////////////////////////////
// DONOR CACHE IMPLEMENTATION
//

//------------------------------------------------------------
// Internal types
//

//
// Cache key type
//

struct sCacheKey 
{
   ObjID obj;
   TraitID trait;

   sCacheKey(ObjID o, TraitID t) :obj(o),trait(t) {}; 
   BOOL operator == (const struct sCacheKey& other) const
   {
      return memcmp(this,&other,sizeof(*this)) == 0; 
   }

};


//
// Least-recently-used (LRU) list
//

class cDonorCacheEntry;
typedef cDList<cDonorCacheEntry, 1> cDonorLRU;
typedef cDListNode<cDonorCacheEntry, 1> cDonorLRUNode; 

//
// List of traits by object
//

typedef cDList<cDonorCacheEntry, 2> cObjEntryList; 
typedef cDListNode<cDonorCacheEntry, 2> cObjEntryListNode; 

//
// Hash of above lists
// 
typedef cScalarHashFunctions<ObjID> cObjIDFuncs; 
typedef cHashTable<ObjID,cObjEntryList*,cObjIDFuncs> cEntriesByObj; 

//
// Trait descriptions and cache accumulations
//
// Dynamic array of sDonorCacheStats structures type definition.
typedef cDynArray<sDonorCacheStats> DonorStats;
// Dynamic array of sTraitDescs structures type definition.
typedef cDynArray<const sTraitDesc*> TraitDescriptions;

//
// Cache entry
//

class cDonorCacheEntry : public cDonorLRUNode, public cObjEntryListNode
{
public:
   sCacheKey key;
   ObjID donor;
   ObjID through;

   cDonorCacheEntry(ObjID o, TraitID t, ObjID d, ObjID thru)
      : key(o,t),
        donor(d),   
        through(thru)
   {};
};

//
// Hash functions
//
struct cCacheKeyHashFns
{
   static unsigned Hash(const sCacheKey* p)
   {
      return (p->trait << 16) + p->obj; 
   }

   static BOOL IsEqual(const sCacheKey* p1, const sCacheKey* p2)
   {
      return *p1 == *p2;
   }
};

//
// Cache entry hash set
//

//#define USE_HASHPP
#ifdef USE_HASHPP
typedef cHashTable<sCacheKey*,cDonorCacheEntry*,cCacheKeyHashFns> cDonorTableBase;
#else
typedef cHashSet<cDonorCacheEntry*,sCacheKey*,cCacheKeyHashFns> cDonorTableBase; 
#endif 

class cDonorTable : public cDonorTableBase
{
public:

#ifdef USE_HASHPP 
   void Insert(cDonorCacheEntry* e)
   {
      cDonorTableBase::Insert(&e->key,e); 
   }

   void Remove(cDonorCacheEntry* e)
   {
      Delete(&e->key); 
   }

#else

   void Clear(int size)
   {
      SetEmpty(); 
      Resize(size); 
   }

   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)&((cDonorCacheEntry*)p)->key;
   }


#endif 

};

#undef USE_HASHPP


//------------------------------------------------------------
// IMPLEMENTATION CLASS
//

class cDonorCache : public IDonorCache
{
   DECLARE_AGGREGATION(cDonorCache);

public:
   cDonorCache(IUnknown* pOuter);
   virtual ~cDonorCache();

   //
   // IDonorCache methods
   //

   STDMETHOD_(TraitID,NewTrait)(const sTraitDesc* tDesc);
   STDMETHOD(SetDonor)(ObjID obj, TraitID trait, ObjID donor, ObjID thru);
   STDMETHOD_(BOOL,GetDonor)(ObjID obj, TraitID trait, ObjID* donor, ObjID* through);
   STDMETHOD(Clear)();
   STDMETHOD(Flush)(ObjID obj, TraitID trait);  
   STDMETHOD(FlushObjSet)(IObjectQuery* objs, TraitID traits);
   STDMETHOD(SetParams)(const sDonorCacheParams* params);
   STDMETHOD(GetParams)(sDonorCacheParams* params);
   STDMETHOD(GetTotalStats)(sDonorCacheStats* stats);
   STDMETHOD(GetStatsByTrait)(TraitID trait, sDonorCacheStats* stats);
   STDMETHOD(GetByTraitName)(const char* name, sDonorCacheStats* stats);
   STDMETHOD(GetMaxID)();
   STDMETHOD(GetCacheEntries)();


protected:
   //
   // Aggregate protocol
   //
   HRESULT Init();
   HRESULT End();

   //
   // Internals
   //
   
   void Add(cDonorCacheEntry* e);
   void Drop(cDonorCacheEntry* e); // drop an entry
   void Touch(cDonorCacheEntry* e);

private:
   cDonorLRU LRU;
   cDonorTable Table;
   cEntriesByObj ByObj; 

   sDonorCacheStats Stats;
   sDonorCacheParams Params;
   TraitID NextID;
   // Donor statistics dynamic array of sDonorCacheStats structures.
   DonorStats  StatsByID;
   // Trait descriptions dynamic array of STraitDesc structures.
   TraitDescriptions TraitDescs;
   ulong StatEntries;
};



#endif // __TRCACHE__H
