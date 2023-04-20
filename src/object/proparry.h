/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proparry.h,v 1.9 2000/02/24 23:41:02 mahk Exp $


#pragma once  
#ifndef __PROPARRY_H
#define __PROPARRY_H

#include <objtype.h>
#include <property.h>
#include "propsto_.h"
#include <dataops.h>
#include <dataops_.h>
#include <packflag.h>
#include <ObjArray.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
// ARRAY PROPERTY STORE IMPLEMENTATION 
//

template <class OPS, ulong flags = kOAF_Default> 
class cArrayPropertyStore: public cPropertyStore<OPS>
{
protected:   


   
public:
   cArrayPropertyStore()
      : mArray(),       
        mInUse(MaxObj()-MinObj()), 
        m_pSink(NULL)
   {
      AutoConnect(); 
   }

   ~cArrayPropertyStore()
   {
      AutoDisconnect(); 
   }


   //
   // Helpers
   // 

   tObjIdx MinObj() const { return mArray.Bounds().min; }; 
   tObjIdx MaxObj() const { return mArray.Bounds().max; }; 

   
   BOOL InUse(ObjID obj) const { return mInUse.IsSet(obj - MinObj()); };
   void SetInUse(ObjID obj)  { mInUse.Set(obj - MinObj()); }; 
   void ClearInUse(ObjID obj)  { mInUse.Clear(obj - MinObj()); }; 

   BOOL InBounds(ObjID obj) const
   { 
      tObjIdx idx = obj; 
      return idx >= MinObj() && idx < MaxObj();
   }      

   //
   // Auto-resizing
   // 

   void AutoConnect(); 
   void AutoDisconnect(); 

   void Resize(const sObjBounds& bounds)
   {
      tObjIdx oldMin = MinObj(); 
      tObjIdx oldMax = MaxObj(); 

      // Resize our vector
      tResult result = mArray.Resize(bounds); 

      if (result != S_OK) 
         return;  // No work to do.

      // If min changes, we have to shift all our bits around
      if (oldMin != MinObj())
      {
         // Make a copy of the vector
         cPackedBoolSet tempVec(oldMax-oldMin);
         tempVec.CopyFrom(mInUse); 

         // Resize it
         mInUse.Resize(MaxObj()-MinObj());          
         mInUse.ClearAll(); 

         // Now copy over the temp vector bit by bit
         tObjIdx start = (oldMin > bounds.min) ? oldMin : bounds.min; 
         tObjIdx end   = (oldMax < bounds.max) ? oldMax : bounds.max; 

         for (int i = start; i < end; i++)
            if (tempVec.IsSet(i - oldMin))
               SetInUse(i);
      }
      else
         mInUse.Resize(MaxObj()-MinObj());
   }


   
   //
   // FAST ACCESS PATH 
   //
   
   sDatum& operator[](ObjID obj) const { Assert_(InBounds(obj)); return mArray[obj]; }; 


   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const 
   {
      static sPropertyStoreDesc desc = { "Array" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      sDatum val; 
      if (InBounds(obj) && !InUse(obj))
      {
         val = this->mOps.New(); 
         mArray[obj] = val; 
         SetInUse(obj); 
      }

      return val; 
   }

   STDMETHOD(Delete)(ObjID obj)
   {
      if (InBounds(obj) && InUse(obj))
      {
         this->mOps.Delete(mArray[obj]); 
         mArray[obj] = sDatum(NULL); 
         ClearInUse(obj); 
         return S_OK; 
      }
      return S_FALSE; 
   }

   STDMETHOD(Release)(ObjID obj, sDatum *pval)
   {
      if (InBounds(obj) && InUse(obj))
      {
         *pval = mArray[obj];
         mArray[obj] = sDatum(NULL); 
         ClearInUse(obj); 
         return S_OK; 
      }
      return S_FALSE; 
   }
      

   STDMETHOD_(BOOL,Relevant)(ObjID obj) const
   {
      return InBounds(obj) && InUse(obj); 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pval) const
   {
      if (InBounds(obj) && InUse(obj))
      {
         *pval = mArray[obj];
         return TRUE; 
      }
      return FALSE; 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      if (!InBounds(obj))
         return E_FAIL; 

      if (InUse(obj))
      {
         this->mOps.Copy(&mArray[obj],val); 
         return S_FALSE;
      }
      else
      {
         mArray[obj] = this->mOps.CopyNew(val);
         SetInUse(obj); 
         return S_OK; 
      }
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      if (!InBounds(targ) || !InBounds(src) || !InUse(src))
         return sDatum(); 

      sDatum val = mArray[src]; 

      if (InUse(targ))
      {
         this->mOps.Copy(&mArray[targ],val); 
      }
      else
      {
         mArray[targ] = this->mOps.CopyNew(val); 
         SetInUse(targ); 
      }
      return val; 
   }

   STDMETHOD(Reset)()
   {
      for (int i = MinObj(); i < MaxObj(); i++)
         if (InUse(i))
         {
            this->mOps.Delete(mArray[i]); 
            mArray[i] = sDatum(NULL); 
            ClearInUse(i); 
         }
      return S_OK; 
   }

   STDMETHOD(IterStart)(sPropertyObjIter* piter) const
   {
      piter->next = MinObj(); 

      return S_OK; 
   }

   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* piter, ObjID* obj, sDatum* value) const
   {
      while (piter->next < MaxObj() && !InUse(piter->next))
         piter->next++; 
      if (piter->next >= MaxObj())
         return FALSE; 

      if (obj)
         *obj = piter->next;
      if (value)
         *value = mArray[piter->next]; 
      piter->next++; 

      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const
   {
      return S_OK; 
   }

protected: 
   typedef cObjArray<sDatum,flags|kOAF_NoAutoResize> cDatumVec;


   // @NOTE: Order here is very important for construction 
   cDatumVec mArray;
 
   cPackedBoolSet mInUse; 

   IObjIDSink* m_pSink; 
};

//------------------------------------------------------------
// Generic version
//

class cGenericArrayPropertyStore : public cArrayPropertyStore<cDelegatingDataOps>
{

public:
   cGenericArrayPropertyStore()
      : cArrayPropertyStore<cDelegatingDataOps>()
   {
      this->mOps.InitDelegation(this); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      this->mOps.SetOps(ops); 
      return S_OK; 
   }

}; 

//////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cArrayPropObjIDSink
//

template <class STORE> 
class cArrayPropObjIDSink: public cObjIDSink
{

public:
   
   cArrayPropObjIDSink(STORE* pStore) : m_pStore(pStore) {}; 


   virtual void OnObjIDSpaceResize(const sObjBounds& bounds)
   {
      m_pStore->Resize(bounds); 
   }

private:

   STORE* m_pStore; 


};

//////////////////////////////////////////////////////////////////////////////
//
// AutoConnection support for array
//
//

template <class OPS, ulong flags>
inline void cArrayPropertyStore<OPS,flags>::AutoConnect()
{
   AutoAppIPtr(ObjIDManager); 

   // Resize the array based on the current size of the ObjID space
   Resize(pObjIDManager->GetObjIDBounds()); 

   m_pSink = new cArrayPropObjIDSink< cArrayPropertyStore<OPS,flags> >(this); 
      
   pObjIDManager->Connect(m_pSink); 
  
}

///////////////////////////////////////

template <class OPS, ulong flags>
inline void cArrayPropertyStore<OPS,flags>::AutoDisconnect()
{
   if (m_pSink)
   {
      AutoAppIPtr(ObjIDManager); 
      pObjIDManager->Disconnect(m_pSink); 

      SafeRelease(m_pSink);       
   }
}



#include <undbmem.h>

#endif // __PROPARRY_H
