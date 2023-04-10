/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/pstordel.h,v 1.1 1998/03/26 13:53:06 mahk Exp $
#pragma once  
#ifndef __PSTORDEL_H
#define __PSTORDEL_H

#include <propstor.h>

class cDelegatingPropertyStore : public cCTDelegating<IPropertyStore>
{
   IPropertyStore* mpStore; 
public:

   cDelegatingPropertyStore(IPropertyStore* inner, IUnknown* outer)
      : mpStore(inner), cCTDelegating<IPropertyStore>(outer)
   {
      if (mpStore)
         mpStore->AddRef(); 
   }

   ~cDelegatingPropertyStore() 
   { 
      SafeRelease(mpStore); 
   }

   void SetStore(IPropertyStore* store)
   {
      // Note order of addref/release so that nothing gets dropped on the floor...
      if (store)
         store->AddRef();  
      SafeRelease(mpStore); 
      mpStore = store; 
   }

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const { return mpStore->Describe(); }; 
   STDMETHOD(SetID)(PropertyID id) { return mpStore->SetID(id); }; 
   STDMETHOD_(sDatum,Create)(ObjID obj) { return mpStore->Create(obj);}; 
   STDMETHOD(Delete)(ObjID obj) { return mpStore->Delete(obj); }; 
   STDMETHOD_(BOOL,Relevant)(ObjID obj) const { return mpStore->Relevant(obj); }; 
   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pdat ) const { return mpStore->Get(obj,pdat); };  
   STDMETHOD(Set)(ObjID obj, sDatum val) { return mpStore->Set(obj,val); }; 
   STDMETHOD_(sDatum, Copy)(ObjID targ, ObjID src) { return mpStore->Copy(targ,src); };

   STDMETHOD(Reset)() { return mpStore->Reset(); }; 

   STDMETHOD(IterStart)(sPropertyObjIter* iter) const { return mpStore->IterStart(iter); };  
   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* iter, 
                             ObjID* obj, sDatum* value) const 
   { return mpStore->IterNext(iter,obj,value); }; 

   STDMETHOD(IterStop)(sPropertyObjIter* iter) const { return mpStore->IterStop(iter); }; 

   STDMETHOD_(BOOL,GetCopy)(ObjID obj, sDatum* val) { return mpStore->GetCopy(obj,val); }; 
   STDMETHOD(ReleaseCopy)(ObjID obj, sDatum val) { return mpStore->ReleaseCopy(obj,val); }; 

   STDMETHOD(WriteObj)(ObjID obj, IDataOpsFile* file)  { return mpStore->WriteObj(obj,file); };
   STDMETHOD(ReadObj) (ObjID obj, IDataOpsFile* file, int version) { return mpStore->ReadObj(obj,file,version); };
   STDMETHOD_(int,WriteVersion)() { return mpStore->WriteVersion();};
   STDMETHOD_(IDataOps*,GetOps)()  { return mpStore->GetOps(); }; 
   STDMETHOD(SetOps)(IDataOps* ops) { return mpStore->SetOps(ops); }; 

}; 

#endif // __PSTORDEL_H

