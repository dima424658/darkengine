/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propstor.h,v 1.1 1998/03/26 13:53:01 mahk Exp $
#pragma once  
#ifndef __PROPSTOR_H
#define __PROPSTOR_H

#include <proptype.h>
#include <dataopst.h>

F_DECLARE_INTERFACE(IPropertyStore); 
//
// Property Storage Interface
//

#undef INTERFACE
#define INTERFACE 
DECLARE_INTERFACE_(IPropertyStore,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 
   
   //
   // Describe our implementation, intended for debugging
   //
   STDMETHOD_(const sPropertyStoreDesc*,Describe)(THIS) CONSTFUNC PURE; 

   //
   // Set our property id, done by the IProperty object 
   //
   STDMETHOD(SetID)(THIS_ PropertyID id) PURE; 

   //
   // Create/Delete entries
   //
   STDMETHOD_(sDatum,Create)(THIS_ ObjID obj) PURE;
   STDMETHOD(Delete)(THIS_ ObjID obj) PURE; 

   // 
   // Check for the presence of an object
   //
   STDMETHOD_(BOOL,Relevant)(THIS_ ObjID obj) CONSTFUNC PURE; 

   //
   // Get/Set the value for an object 
   //
   STDMETHOD_(BOOL,Get)(THIS_ ObjID obj, sDatum* ) CONSTFUNC PURE; 

   // S_OK if newly added, S_FALSE if already present 
   STDMETHOD(Set)(THIS_ ObjID obj, sDatum val) PURE;

   //
   // Copy an entry
   //
   STDMETHOD_(sDatum, Copy)(THIS_ ObjID targ, ObjID src) PURE; 

   //
   // Delete all entries
   //
   STDMETHOD(Reset)(THIS) PURE; 

   //
   // Iterate across all entries
   //
   STDMETHOD(IterStart)(THIS_ sPropertyObjIter* iter) CONSTFUNC PURE; 
   STDMETHOD_(BOOL,IterNext)(THIS_ sPropertyObjIter* iter, 
                             ObjID* obj, sDatum* value) CONSTFUNC PURE; 
   STDMETHOD(IterStop)(THIS_ sPropertyObjIter* iter) CONSTFUNC PURE; 

   //
   // Get a copy of a value
   //
   STDMETHOD_(BOOL,GetCopy)(THIS_ ObjID obj, sDatum* val) PURE; 
   // Release the copy you got, must be called when you're finished with the value
   STDMETHOD(ReleaseCopy)(THIS_ ObjID obj, sDatum val) PURE; 


   //
   // Read/write the value for an object 
   //
   STDMETHOD(WriteObj)(THIS_ ObjID obj, IDataOpsFile* file) PURE; 
   STDMETHOD(ReadObj) (THIS_ ObjID obj, IDataOpsFile* file , int version) PURE; 

   // version of the data that we are writing out
   STDMETHOD_(int,WriteVersion)(THIS) PURE; 

   //
   // Access my DataOps, these may fail in cases where the store 
   // doesn't have any, or can't change the ones it has. 
   //
   STDMETHOD_(IDataOps*,GetOps)(THIS) PURE; 
   STDMETHOD(SetOps)(THIS_ IDataOps* ops) PURE; 


}; 

//
// Create a generic property store 
//

IPropertyStore* CreateGenericPropertyStore(ePropertyImpl impl, IDataOps* ops = NULL); 

#endif // __PROPSTOR_H
