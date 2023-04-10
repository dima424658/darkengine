/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propmix.cpp,v 1.7 1998/10/05 17:27:15 mahk Exp $

#include <propmix.h>
#include <propbase.h>

// Must be last header 
#include <dbmem.h>


cMixedPropertyStore::cMixedPropertyStore(IPropertyStore* abstract, IPropertyStore* concrete)
{
   abstract->AddRef(); 
   concrete->AddRef(); 
   mStores[kAbstract] = abstract;
   mStores[kConcrete] = concrete;
   IDataOps* ops = concrete->GetOps(); 
   if (ops)
   {
      abstract->SetOps(ops);
      SafeRelease(ops); 
   }
}

cMixedPropertyStore::~cMixedPropertyStore()
{
   SafeRelease(mStores[kAbstract]);
   SafeRelease(mStores[kConcrete]); 
}

//
// METHODS 
//

STDMETHODIMP_(const sPropertyStoreDesc*) cMixedPropertyStore::Describe() const 
{
   static sPropertyStoreDesc desc = { "Hybrid implementation" }; 
   return &desc; 
   
}

STDMETHODIMP cMixedPropertyStore::SetID(PropertyID id)
{
   mStores[kAbstract]->SetID(id); 
   mStores[kConcrete]->SetID(id); 
   return S_OK; 
}

STDMETHODIMP_(sDatum) cMixedPropertyStore::Create(ObjID obj)
{
   return Store(obj)->Create(obj); 
}

STDMETHODIMP cMixedPropertyStore::Delete(ObjID obj)
{
   return Store(obj)->Delete(obj); 
}

STDMETHODIMP_(BOOL) cMixedPropertyStore::Relevant(ObjID obj) const
{
   return Store(obj)->Relevant(obj); 
}

STDMETHODIMP_(BOOL) cMixedPropertyStore::Get(ObjID obj, sDatum* pdat) const
{
   return Store(obj)->Get(obj,pdat); 
}

STDMETHODIMP cMixedPropertyStore::Set(ObjID obj, sDatum val)
{
   return Store(obj)->Set(obj,val); 
}

STDMETHODIMP_(sDatum) cMixedPropertyStore::Copy(ObjID targ, ObjID src)
{
   // could copy across stores, so have to do it stupidly
   sDatum dat; 
   if (Get(src,&dat))
      Set(targ,dat);
   return dat; 
}

STDMETHODIMP cMixedPropertyStore::Reset()
{
   mStores[kAbstract]->Reset(); 
   mStores[kConcrete]->Reset(); 
   return S_OK; 
}

STDMETHODIMP cMixedPropertyStore::IterStart(sPropertyObjIter* iter) const
{
   // Start doing the abstract ones 
   iter->next = kConcrete; 
   sPropertyObjIter* subiter = new sPropertyObjIter; 
   iter->state[0] = subiter;
   mStores[iter->next]->IterStart(subiter); 
   return S_OK; 
}

STDMETHODIMP_(BOOL) cMixedPropertyStore::IterNext(sPropertyObjIter* iter, 
                                                  ObjID* obj, sDatum* value) const
{
   sPropertyObjIter* subiter = (sPropertyObjIter*) iter->state[0];
   if (mStores[iter->next]->IterNext(subiter,obj,value))
      return TRUE; 
   
   // if we finished the concretes, start on the abstracts 
   if (iter->next == kConcrete)
   {
      iter->next = kAbstract; 
      mStores[kConcrete]->IterStop(subiter); 
      mStores[kAbstract]->IterStart(subiter); 
      return mStores[kAbstract]->IterNext(subiter,obj,value); 
   }
   
   return FALSE;     
}


STDMETHODIMP cMixedPropertyStore::IterStop(sPropertyObjIter* iter) const
{
   sPropertyObjIter* subiter = (sPropertyObjIter*) iter->state[0];
   delete subiter;
   iter->state[0] = NULL; 
   return S_OK; 
}

STDMETHODIMP_(BOOL) cMixedPropertyStore::GetCopy(ObjID obj, sDatum* val) 
{
   return Store(obj)->GetCopy(obj,val); 
} 

STDMETHODIMP cMixedPropertyStore::ReleaseCopy(ObjID obj, sDatum val)
{
   return Store(obj)->ReleaseCopy(obj,val); 
}


STDMETHODIMP_(int) cMixedPropertyStore::WriteVersion()
{
   // Really your data ops shouldn't be out of sync
   return mStores[kConcrete]->WriteVersion(); 
}

STDMETHODIMP cMixedPropertyStore::WriteObj(ObjID obj, IDataOpsFile* file)
{
   return Store(obj)->WriteObj(obj,file); 
}

STDMETHODIMP cMixedPropertyStore::ReadObj (ObjID obj, IDataOpsFile* file , int version)
{
   return Store(obj)->ReadObj(obj,file,version); 
} 

STDMETHODIMP_(IDataOps*) cMixedPropertyStore::GetOps()
{
   return mStores[kConcrete]->GetOps(); 
}

STDMETHODIMP cMixedPropertyStore::SetOps(IDataOps* ops)
{
   mStores[kAbstract]->SetOps(ops);
   mStores[kConcrete]->SetOps(ops);
   return S_OK; 
}



