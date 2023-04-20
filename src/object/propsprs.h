/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propsprs.h,v 1.4 1998/10/06 10:45:44 CCAROLLO Exp $
#pragma once  
#ifndef __PROPSPRS_H
#define __PROPSPRS_H

#include <propsto_.h>
#include <dataops_.h>
#include <hashpp.h>
#include <propinst.h>
#include <propbase.h>

#define PUSH_BLAME() PROP_PUSH_BLAME("Sparse Hash Property Store")
#define POP_BLAME() PROP_POP_BLAME()

////////////////////////////////////////////////////////////
// "Sparse Hash" property store, essentially a hashtable shared by
// many (presumably small) properties.  
//

//
// Obj,Prop pair that we use as a key for our hash table 
//

struct sObjPropPair 
{
   ObjID obj;
   PropertyID prop; 

   sObjPropPair(ObjID o = NULL, PropertyID p = PROPID_NULL) :obj(o),prop(p) {}; 
}; 

//
// Hash functions 
//

struct sObjPropHashFuncs
{
   static uint Hash(const sObjPropPair& pair) { return HashThing(&pair,sizeof(pair)); };
   static BOOL IsEqual(const sObjPropPair& p1, const sObjPropPair& p2) 
   { return p1.obj == p2.obj && p1.prop == p2.prop; }; 
};

//
// The hash table itself 
//

class cObjPropDatumHashTable : public cHashTable<sObjPropPair,sDatum,sObjPropHashFuncs>
{
public:
   // the default table 
   static cObjPropDatumHashTable& DefaultTable(); 

   // I'm to cool to add this to hashpp.h
   long TableSize() { return size; }; 
}; 

////////////////////////////////////////////////////////////
//
// cSparseHashPropertyStore
//
//////////////////////////////////////////////////////////////

template <class OPS> 
class cSparseHashPropertyStore: public cPropertyStore<OPS>
{
   
public:
   typedef cObjPropDatumHashTable cTable; 
   typedef sObjPropPair sKey; 

   //
   // Construction 
   //

   cSparseHashPropertyStore(cTable* table = NULL)
      : mTable((table) ? *table : cTable::DefaultTable())
   {
      
   }

   ~cSparseHashPropertyStore()
   {
   }

   //
   // FAST ACCESS PATH (As if you were likely to want one) 
   //

   inline cTable& GetTable() { return mTable; }; 

   //
   // Helpers
   //

   inline sKey Key(ObjID obj) const { return sObjPropPair(obj,this->mID); }; 

   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const
   {
      static sPropertyStoreDesc desc = { "Sparse Hash Table" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      sKey key = Key(obj); 
      sDatum val; 
      if (!mTable.Lookup(key,&val))
      {
         val = this->mOps.New(); 
         PUSH_BLAME(); 
         mTable.Set(key,val); 
         POP_BLAME(); 
      }

      return val; 
   }

   STDMETHOD(Delete)(ObjID obj)
   {
      sKey key = Key(obj); 
      sDatum val;
      if (mTable.Lookup(key,&val))
      {
         this->mOps.Delete(val);    
         mTable.Delete(key);
         return S_OK; 
         
      }
      return S_FALSE; 
   }

   STDMETHOD_(BOOL,Relevant)(ObjID obj)  const
   {
      return mTable.HasKey(Key(obj)); 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pval) const 
   {
      return mTable.Lookup(Key(obj),pval); 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      sKey key = Key(obj); 
      HRESULT retval = S_OK; 
      sDatum setval; 
      if (mTable.Lookup(key,&setval))
      {
         this->mOps.Copy(&setval,val);
         retval = S_FALSE; 
      }
      else
         setval = this->mOps.CopyNew(val);
      PUSH_BLAME(); 
      mTable.Set(key,setval);
      POP_BLAME(); 
      return retval; 
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      sDatum srcval; 
      if (mTable.Lookup(Key(src),&srcval))
      {
         sDatum targval = this->mOps.CopyNew(srcval);
         PUSH_BLAME(); 
         mTable.Set(Key(targ),targval); 
         POP_BLAME(); 
         return srcval;
      }
      return sDatum(0); 
   }

   STDMETHOD(Reset)()
   {
      cTable::cIter iter;  
      for (iter = mTable.Iter(); !iter.Done(); iter.Next())
         if (iter.Key().prop == this->mID)
         {
            this->mOps.Delete(iter.Value());
            mTable.Delete(iter.Key()); 
         }
      return S_OK; 
   }

   STDMETHOD(IterStart)(sPropertyObjIter* piter) const 
   {
      cTable::cIter& iter = *(cTable::cIter*)piter; 
      Assert_(sizeof(*piter) >= sizeof(iter)); 
      
      iter = mTable.Iter(); 
      return S_OK; 
   }

   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* piter, ObjID* obj, sDatum* value) const
   {
      cTable::cIter& iter = *(cTable::cIter*)piter; 

      // Skip over entries that aren't us. 
      while (!iter.Done() && iter.Key().prop != this->mID)
         iter.Next(); 

      if (iter.Done())
         return FALSE; 

      if (obj)
         *obj = iter.Key().obj; 
      if (value)
         *value = iter.Value(); 

      iter.Next(); 
      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* iter) const 
   {
      return S_OK; 
   }

protected: 
   cTable& mTable; 

};

//------------------------------------------------------------
// Generic version
//


class cGenericSparseHashPropertyStore : public cSparseHashPropertyStore<cDelegatingDataOps>
{
public:
   cGenericSparseHashPropertyStore()
   {
      this->mOps.InitDelegation(this); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      this->mOps.SetOps(ops); 
      return S_OK; 
   }

}; 


#endif // __PROPSPRS_H

