/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propcary.h,v 1.5 2000/02/24 23:41:04 mahk Exp $
#pragma once  
#ifndef __PROPCARY_H
#define __PROPCARY_H

#include <propsto_.h>
#include <dataops_.h>
#include <propbase.h>
#include <dynarray.h>
#include <osysbase.h>
#include <objarray.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
// "Compact Array" property store
//
// Basically, an array of the N objects that have this property 
// 
// * Fast iteration, w/ easy back door
// * Lookup as fast as you're willing to make it 
// 
////////////////////////////////////////////////////////////


//
// "Index map" interface.  The first template argument to the store 
// is a class with this interface.  
// 

class cObjIndexMap 
{
public:
   // Get the index of an object, or zero if not present 
   virtual int GetIndex(ObjID obj) const = 0; 

   // Set the index of an object, idx zero means remove
   virtual void SetIndex(ObjID obj, int idx) = 0; 

   // clear the map
   virtual void Clear() = 0; 
}; 


////////////////////////////////////////////////////////////
// cCompactArrayPropertyStore 
//

template <class MAP, class OPS> 
class cCompactArrayPropertyStore: public cPropertyStore<OPS>
{

   
public:
   enum { kMinIdx = 1 }; 

   typedef cDynArray<sDatum> cValues;
   typedef cDynArray<ObjID> cObjIDs; 

   //
   // constructor
   // 
   cCompactArrayPropertyStore()
   {
      // go past index #0 
      mValues.Append(sDatum());  
      mObjIDs.Append(OBJ_NULL); 
   }


   //
   // Helpers
   // 

   int NewIdx()
   {
      for (int i = kMinIdx; i < mObjIDs.Size(); i++)
      {
         if (mObjIDs[i] == OBJ_NULL)
            return i; 
      }
      int idx = mObjIDs.Append(OBJ_NULL); 
      while (mValues.Size() <= idx)
         mValues.Append(sDatum()); 
      return idx; 
   }

   //
   // FAST ACCESS PATH 
   //

   cValues& Values() { return mValues; }; 
   sDatum& operator[](int idx) { return mValues[idx]; }; 

   int Obj2Idx(ObjID obj) const {  return mMap.GetIndex(obj); }; 
   int Idx2Obj(int idx) const { return (idx < mObjIDs.Size()) ? mObjIDs[idx] : OBJ_NULL; }; 

   int MaxIdx() const { return mValues.Size(); };

   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const 
   {
      static sPropertyStoreDesc desc = { "Compact Array" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      int idx = Obj2Idx(obj);
      if (idx == 0)
      {
         idx = NewIdx(); 
         mMap.SetIndex(obj,idx); 
         mObjIDs[idx] = obj;
         mValues[idx] = mOps.New(); 
      }
      return mValues[idx]; 
   }

   STDMETHOD(Delete)(ObjID obj)
   {
      int idx = Obj2Idx(obj); 
      if (idx)
      {
         mOps.Delete(mValues[idx]);    
         mValues[idx] = sDatum(); 
         mObjIDs[idx] = OBJ_NULL; 
         mMap.SetIndex(obj,0); 
         return S_OK; 
      }
      return S_FALSE; 
   }

   STDMETHOD_(BOOL,Relevant)(ObjID obj)  const 
   {
      return Obj2Idx(obj) != 0; 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pval) const 
   {
      int idx = Obj2Idx(obj); 
      if (idx)
      {
         *pval = mValues[idx]; 
      }
      return idx != 0; 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      int idx = Obj2Idx(obj);
      if (idx) 
      {
         mOps.Copy(&mValues[idx],val); 
         return S_FALSE;
      }
      else
      {
         idx = NewIdx();
         mMap.SetIndex(obj,idx);
         mValues[idx] = mOps.CopyNew(val); 
         mObjIDs[idx] = obj; 
         return S_OK; 
      }
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      int srcidx = Obj2Idx(src);
      if (srcidx == 0) return NULL; 

      int targidx = Obj2Idx(targ); 
      sDatum srcval = mValues[srcidx]; 
      if (targidx)
      {
         mOps.Copy(&mValues[targidx],srcval); 
      }
      else
      {
         targidx = NewIdx();
         mMap.SetIndex(targ,targidx);
         mValues[targidx] = mOps.CopyNew(srcval); 
         mObjIDs[targidx] = targ; 
      }
      return mValues[targidx]; 
   }

   STDMETHOD(Reset)()
   {
      for (int i = 0; i < mObjIDs.Size(); i++)
         if (mObjIDs[i] != OBJ_NULL)
         {
            mOps.Delete(mValues[i]);
            mValues[i] = sDatum(); 
            mObjIDs[i] = OBJ_NULL; 
         }

      mMap.Clear(); 
      return S_OK; 
   }

   STDMETHOD(IterStart)(sPropertyObjIter* piter) const 
   {
      piter->next = kMinIdx; 
      return S_OK; 
   }

   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* piter, ObjID* obj, sDatum* value) const 
   {
      while (piter->next < mObjIDs.Size() && mObjIDs[piter->next] == OBJ_NULL)
         piter->next++; 

      if (piter->next >= mObjIDs.Size()) 
         return FALSE; 

      if (obj)
         *obj = mObjIDs[piter->next]; 
      if (value)
         *value = mValues[piter->next]; 
      piter->next++; 
      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const 
   {
      return S_OK; 
   }

protected: 
   cValues mValues;
   cObjIDs mObjIDs; 
   MAP mMap; 
};

//------------------------------------------------------------
// Generic version
//

//
// Generic slow index map.  Doesn't actually store anything, just searches through 
// the objid array
//

class cSlowObjIndexMap : public cObjIndexMap
{
   
public:
   typedef cDynArray<ObjID> cObjIDs; 
   void SetArray(cObjIDs* ids) { mpObjIDs = ids; }; 

   int GetIndex(ObjID obj) const 
   {
      cObjIDs& ids = *mpObjIDs; // ptrs to dynarrays are nasty to work with
      for (int i = 1; i < ids.Size(); i++)
         if (ids[i] == obj)
            return i; 
      return 0; 
   }

   void SetIndex(ObjID obj, int idx) {}; 
   void Clear() { }; 

protected:
   cObjIDs* mpObjIDs; 
};


class cGenericCompactArrayPropertyStore 
   : public cCompactArrayPropertyStore<cSlowObjIndexMap,cDelegatingDataOps>
{
public:
   cGenericCompactArrayPropertyStore()
   {
      mOps.InitDelegation(this); 
      mMap.SetArray(&mObjIDs); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      mOps.SetOps(ops); 
      return S_OK; 
   }

}; 

//------------------------------------------------------------
// A few choice index maps
//

#include <hashpp.h>

class cHashObjIndexMap : public cObjIndexMap
{
protected:
   typedef cHashTable<ObjID,int,cHashFunctions> cTable;
   cTable mTable;

public:
   int GetIndex(ObjID obj) const 
   {
      int retval = 0;
      mTable.Lookup(obj,&retval);
      return retval; 
   }

   void SetIndex(ObjID obj, int idx)
   {
      if (idx == 0)
         mTable.Delete(obj);
      else
         mTable.Set(obj,idx); 
   }

   void Clear()
   {
      mTable.Clear(); 
   }

}; 

template <ulong flags = kOAF_NoAbstracts>
class cArrayObjIndexMapBase : public cObjIndexMap
{
protected:
   cObjArray<int,flags> mArray;

public:
 
   int GetIndex(ObjID obj) const 
   {
      return mArray[obj]; 
   }

   void SetIndex(ObjID obj, int idx)
   {
      mArray[obj] = idx; 
   }

   void Clear()
   {
      mArray.Clear(); 
   }

}; 

typedef cArrayObjIndexMapBase<> cArrayObjIndexMap; 




#include <undbmem.h>

#endif // __PROPCARY_H




