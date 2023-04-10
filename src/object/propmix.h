/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propmix.h,v 1.3 1998/03/26 13:52:48 mahk Exp $
#pragma once  
#ifndef __PROPMIX_H
#define __PROPMIX_H

#include <propsto_.h>

////////////////////////////////////////////////////////////
// "Mixed" Property store
//
// A simple multiplexer that uses different stores for abstract & concrete
// objects
//
// Note that it uses a sneaky bit-shifting trick to avoid doing any compares
// when choosing which store to use.  There is currently no version that 
// is templatized on the two property store classes, because the above optimization 
// requires virtual function calls to work, and thus precludes templatization. 
//

class cMixedPropertyStore : public cUnknownPropertyStore
{
public:


   enum 
   {
      kConcrete,
      kAbstract,
      kNumStores
   }; 

   //
   // Construction
   // 

   cMixedPropertyStore(IPropertyStore* abstract, IPropertyStore* concrete);

   ~cMixedPropertyStore();

   //
   // Helpers
   // 

   // Here's the sneaky trick
   inline int StoreIdx(ObjID obj) const 
   { 
      // Shift the sign bit down
      return (((ulong)obj) >> 31) & 1; 
   }

   inline IPropertyStore* Store(ObjID obj) const 
   {
      return mStores[StoreIdx(obj)]; 
   }

   //
   // Methods
   //

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const;
   STDMETHOD(SetID)(PropertyID id); 
   STDMETHOD_(sDatum,Create)(ObjID obj);
   STDMETHOD(Delete)(ObjID obj); 
   STDMETHOD_(BOOL,Relevant)(ObjID obj) const ; 
   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* ) const ; 
   STDMETHOD(Set)(ObjID obj, sDatum val);
   STDMETHOD_(sDatum, Copy)(ObjID targ, ObjID src); 
   STDMETHOD(Reset)(); 
   STDMETHOD(IterStart)(sPropertyObjIter* iter) const; 
   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* iter, 
                             ObjID* obj, sDatum* value) const ; 
   STDMETHOD(IterStop)(sPropertyObjIter* iter) const ; 
   STDMETHOD_(BOOL,GetCopy)(ObjID obj, sDatum* val)  ; 
   STDMETHOD(ReleaseCopy)(ObjID obj, sDatum val); 
   STDMETHOD_(int,WriteVersion)(); 
   STDMETHOD(WriteObj)(ObjID obj, IDataOpsFile* file); 
   STDMETHOD(ReadObj) (ObjID obj, IDataOpsFile* file , int version); 
   STDMETHOD_(IDataOps*,GetOps)(); 
   STDMETHOD(SetOps)(IDataOps* ops); 

protected:

   IPropertyStore* mStores[kNumStores]; 

}; 


#endif // __PROPMIX_H
