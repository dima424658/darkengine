/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/prophash.h,v 1.10 2000/02/22 20:00:46 toml Exp $
#pragma once

#ifndef PROPHASH_H
#define PROPHASH_H
#include <propsto_.h>
#include <dataops_.h>
#include <propbase.h>
#include <hashpp.h>

////////////////////////////////////////////////////////////
//
// HASH TABLE PROPERTY STORE IMPLEMENTATION 
//
//

#define DEFAULT_TABLE_SIZE 3

#define USE_HASHPP
#ifdef USE_HASHPP 

class cPropertyHashTable : public cHashTable<ObjID,sDatum,cHashFunctions>
{
public:
   cPropertyHashTable() 
      : cHashTable<ObjID,sDatum,cHashFunctions>(DEFAULT_TABLE_SIZE)
   {} 
}; 

#else 
#include <hashset.h>
#include <hshsttem.h>

#include <dbmem.h>

//
// Adapter between hashset and hashtable
//

struct sPropHashRecord 
{
   ObjID obj;
   sDatum dat; 

   sPropHashRecord(ObjID o = 0, sDatum d = sDatum())
      : obj(o),dat(d)
   {
   }
}; 

typedef cHashSet<sPropHashRecord*,ObjID,cHashFunctions> cPropHashSetBase; 


class cPropertyHashTable : public cPropHashSetBase
{
protected:
   typedef sPropHashRecord sRecord; 
   typedef cPropHashSetBase cParent; 

public:
   cPropertyHashTable() 
      : cParent(DEFAULT_TABLE_SIZE)
   {} 

   ~cPropertyHashTable()
   {
      tHashSetHandle hnd; 
      sRecord* rec; 
      for (rec = GetFirst(hnd); rec != NULL; rec = GetNext(hnd))
         delete rec; 
   }

   void Clear(int size = DEFAULT_TABLE_SIZE)
   {
      SetEmpty(); 
      Resize(size); 
   }

   BOOL Lookup(ObjID obj, sDatum* val) const 
   {
      sRecord* rec = Search(obj); 
      if (rec)
      {
         *val = rec->dat; 
         return TRUE; 
      }
      return FALSE; 
   }

   void Insert(ObjID obj, sDatum val)
   {
      sRecord* rec = new sRecord(obj,val); 
      cParent::Insert(rec); 
   }

   void Set(ObjID obj, sDatum val)
   {
      sRecord* rec = Search(obj);
      if (rec)
         rec->dat = val; 
      else 
         Insert(obj,val);
   }

   void Delete(ObjID obj)
   {
      sRecord* rec = Search(obj); 
      if (rec)
      {
         Remove(rec);
         delete rec; 
      }
   }

   BOOL HasKey(ObjID obj) const 
   {
      return Search(obj) != NULL; 
   }

   class cIter
   {
      tHashSetHandle mHnd; 
      sRecord* mpRec; 
      const cPropertyHashTable* mpTable; 

   public:
      cIter(const cPropertyHashTable* tbl) : mpTable(tbl) { mpRec = tbl->GetFirst(mHnd); }; 

      cIter(const cIter& iter) 
         : mpTable(iter.mpTable), mHnd(iter.mHnd), mpRec(iter.mpRec)
      { };

      cIter() : mpRec(NULL),mpTable(NULL) { }; 

      


      BOOL Done() const { return mpRec == NULL; }; 
      void Next() { mpRec = mpTable->GetNext(mHnd); }; 
      ObjID Key() const { return mpRec->obj; }; 
      sDatum Value() const { return mpRec->dat; }; 
      
   }; 

   cIter Iter() const { return cIter(this); }; 
   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)((sRecord*)p)->obj;
   }


}; 

#include <undbmem.h>

#endif 


#undef USE_HASHPP


template <class OPS> 
class cHashPropertyStore: public cPropertyStore<OPS>
{
   
public:
   typedef cPropertyHashTable cTable; 

   //
   // FAST ACCESS PATH 
   //

   cTable& GetTable() { return mTable; }; 

   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const 
   {
      static sPropertyStoreDesc desc = { "Hash Table" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      Assert_(obj != OBJ_NULL);
      sDatum val; 
      if (!mTable.Lookup(obj,&val))
      {
         val = this->mOps.New(); 
         mTable.Insert(obj,val); 
      }

      return val; 
   }

   STDMETHOD(Delete)(ObjID obj)
   {
      sDatum val;
      if (mTable.Lookup(obj,&val))
      {
         this->mOps.Delete(val);    
         mTable.Delete(obj);
         return S_OK; 
         
      }
      return S_FALSE; 
   }

   STDMETHOD_(BOOL,Relevant)(ObjID obj) const 
   {
      return mTable.HasKey(obj); 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pval) const 
   {
      return mTable.Lookup(obj,pval); 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      Assert_(obj != OBJ_NULL);
      HRESULT retval = S_OK; 
      sDatum setval; 
      if (mTable.Lookup(obj,&setval))
      {
         this->mOps.Copy(&setval,val);
         retval = S_FALSE; 
      }
      else
         setval = this->mOps.CopyNew(val);
      mTable.Set(obj,setval);
      return retval; 
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      Assert_(targ != OBJ_NULL);
      sDatum srcval; 
      if (mTable.Lookup(src,&srcval))
      {
         sDatum targval = this->mOps.CopyNew(srcval); 
         mTable.Set(targ,targval); 
         return srcval;
      }
      return sDatum(0); 
   }

   STDMETHOD(Reset)()
   {
      cTable::cIter iter;  
      for (iter = mTable.Iter(); !iter.Done(); iter.Next())
      {
         this->mOps.Delete(iter.Value());
      }
      mTable.Clear(); 
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
      if (iter.Done())
         return FALSE; 
      if (obj)
         *obj = iter.Key(); 
      if (value)
         *value = iter.Value(); 
      iter.Next(); 
      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const 
   {
      return S_OK; 
   }

protected: 
   cTable mTable; 

};

//------------------------------------------------------------
// Generic version
//

class cGenericHashPropertyStore : public cHashPropertyStore<cDelegatingDataOps>
{
public:
   cGenericHashPropertyStore()
   {
      mOps.InitDelegation(this); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      mOps.SetOps(ops); 
      return S_OK; 
   }

}; 




#endif // PROPHASH_H

