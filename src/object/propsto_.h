/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propsto_.h,v 1.2 1998/04/13 21:34:11 mahk Exp $
#pragma once  
#ifndef __PROPSTO__H
#define __PROPSTO__H

#include <propstor.h>

////////////////////////////////////////////////////////////
// 
// USEFUL PROPERTY STORE IMPLEMENTATION CLASSES 
//
//

//
// Generic IUnknown
//
class cUnknownPropertyStore : public cCTUnaggregated<IPropertyStore,&IID_IPropertyStore,kCTU_NoSelfDelete> 
{
public:
   virtual ~cUnknownPropertyStore() {}; 

   void OnFinalRelease() { delete this; }; // self deletion is the norm 
};

//
// Supports generic SetID, nothing else.
//

class cBasePropertyStore : public cUnknownPropertyStore 
{
protected:
   PropertyID mID; 

public:

   STDMETHOD(SetID)(PropertyID id) { mID = id; return S_OK; }; 
}; 



//
// Basic property store, templatized on the dataops implementation class for speed. 
// 

template <class OPS> 
class cPropertyStore : public cBasePropertyStore
{
protected:
   OPS mOps; 

public:
   
   //
   // Default method implementations
   //


   STDMETHOD(WriteObj)(ObjID obj, IDataOpsFile* file) 
   {
      sDatum dat; 
      if (Get(obj,&dat))
         return mOps.Write(dat,file);
      return S_FALSE; 
   }

   STDMETHOD(ReadObj) (ObjID obj, IDataOpsFile* file, int version) 
   {
      sDatum dat = Create(obj); 
      HRESULT retval = mOps.Read(&dat,file, version);
      if (SUCCEEDED(retval))
         Set(obj,dat);
      else
         Delete(obj); 
      return retval; 
   }

   STDMETHOD_(int,WriteVersion)()  { return mOps.Version(); }; 

   STDMETHOD_(IDataOps*,GetOps)() 
   {
      mOps.AddRef();
      return &mOps; 
   }

   STDMETHOD(SetOps)(THIS_ IDataOps* ) 
   {
      // Can't set our ops
      return E_FAIL; 
   }

   STDMETHOD_(BOOL,GetCopy)(THIS_ ObjID obj, sDatum* val) 
   {
      sDatum dat;
      if (!Get(obj,&dat))
         return FALSE; 
      *val = mOps.New(); 
      mOps.Copy(val,dat); 
      return TRUE; 
   }

   STDMETHOD(ReleaseCopy)(THIS_ ObjID, sDatum val) 
   {
      return mOps.Delete(val); 
   }

}; 

#endif // __PROPSTO__H


