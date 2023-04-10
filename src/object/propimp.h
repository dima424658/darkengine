/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propimp.h,v 1.17 2000/01/29 13:24:30 adurant Exp $
//
#pragma once


#ifndef PROPIMP__H
#define PROPIMP__H

#include <objtype.h>
#include <lgassert.h>
#include <dynarray.h>
#include <str.h>
#include <llist.h>

#include <propbase.h>


////////////////////////////////////////////////////////////
// 
// cPropValueFuncs
//
// An interface for creating, copying, terminating and instantiating
// a property's values. 
//
////////////////////////////////////////////////////////////

typedef void PropFile;

typedef BOOL (*PropReadFunc)(PropFile* file, void* buf, int len);
typedef BOOL (*PropWriteFunc)(PropFile* file, void* buf, int len);

class cPropValueFuncs 
{
public:
   // return the current version of the data structure
   virtual uint Version(void); 
   // initalize a value, grabbing resources, etc
   virtual void Init(ObjID obj, void* value) = 0;  
   // Fill in an initalized value with a "default" value, based on 
   // the suggestion, which may be NULL.
   virtual void New(ObjID obj, void* value, void *sug = NULL) = 0; 
   // Terminate a value, freeing resources grabbed by init, etc.
   virtual void Term(ObjID obj, void* value) = 0; 
   // Copy one initialized value to another.
   virtual void Copy(ObjID obj, void* targ, void* src) = 0;  
   // Read a value in from disk using the read func
   virtual void Read(ObjID obj, void* value, uint version, PropReadFunc read, PropFile* file) = 0;
   // write a value to disk using the write func
   virtual void Write(ObjID obj, void* value, PropWriteFunc write, PropFile* file) = 0;
   
};

////////////////////////////////////////////////////////////
//
// cPropertyImpl
//
// Implements the storage for a property.  Cares about nothing about the
// property itself except its size.  All it has to do is maintain
// mappings between ObjID's and blocks of elem_size_ bytes.
//
// Possible implementations of this interface:
//   List of pairs, implemented as an array
//   List of pairs, implemented as a linked list
//   Hash table
//   Dynamic array with one entry for each ObjID.  But how do we
//     represent irrelevant entries (or tell the cPropertyImpl that
//     irrelevant == FALSE?)
//
////////////////////////////////////////////////////////////

class cPropertyImpl
{
public:
   // Constructor; tell it the size of the property
   cPropertyImpl (int elem_size, cPropValueFuncs* f = NULL);

   // Destructor
   virtual ~cPropertyImpl ();

   // Stuffs a pointer to the value into *pptr, returns whether relevant
   virtual BOOL Get (ObjID obj, PropertyID propid, void **pptr) const = 0;

   // Sets obj's value to what's pointed to by ptr
   // return true iff newly relevant
   virtual BOOL Set (ObjID obj, PropertyID propid, void *ptr) = 0;

   // Makes the property no longer relevant for obj, return whether it was there
   virtual BOOL Delete(ObjID obj, PropertyID propid) = 0;

   // Makes a property newly relevant for obj, inits but does not new the value.
   // Returns value ptr.
   virtual void* Create(ObjID obj, PropertyID propid) = 0; 

   // Do any necessary set up for the creation of a new object. 
   virtual void Prep(ObjID , PropertyID) {};

   // Reset the database to empty 
   virtual void Reset(PropertyID id) = 0; 

   // Iterate
   virtual void IterStart(PropertyID id, sPropertyObjIter* iter) const = 0;
   virtual BOOL IterNext(PropertyID id, sPropertyObjIter* iter, ObjID* next, void** value) const
      = 0;
   virtual void IterStop(PropertyID id, sPropertyObjIter* iter) const = 0;


   // Is this property relevant to the given obj?
   virtual BOOL Relevant (ObjID obj, PropertyID propid) const = 0;

   // Set the value funcs
   void SetValueFuncs(cPropValueFuncs* f);
   cPropValueFuncs* GetValueFuncs() { return vfunc_;};
   BOOL SetValueFuncSponsorship(BOOL sponsored) 
   {
      BOOL retval = sponsored_;
      sponsored_ = sponsored;
      return retval;
   };

   

   //
   // Value funcs interface for subclasses
   // (Useful if there's common code)
   void Init(ObjID obj, void* value) { vfunc_->Init(obj,value);}; 
   void New (ObjID obj, void *value, void* sug = NULL) { vfunc_->New(obj,value,sug);}
   void Term(ObjID obj, void* value) { vfunc_->Term(obj,value);};
   void Copy(ObjID obj, void* targ, void* src) 
   {if (targ != src) vfunc_->Copy(obj,targ,src);}
   void Read(ObjID obj, void* v, uint ver, PropReadFunc r, PropFile* f) { vfunc_->Read(obj,v,ver,r,f);}
   void Write(ObjID obj, void* value, PropWriteFunc write, PropFile* file) 
   { vfunc_->Write(obj,value,write,file);} 
   uint Version(void) { return vfunc_->Version(); }

   // Get size of elements
   int ElemSize () const {return elem_size_;}


private:
   // Prevent copying by value
   cPropertyImpl (const cPropertyImpl&) {};
   cPropertyImpl& operator= (const cPropertyImpl&);

protected:
   int elem_size_;

private:
   BOOL sponsored_;
   cPropValueFuncs* vfunc_;
};


////////////////////////////////////////////////////////////
// Create an impl of a particular type size, from the enum 
// 
EXTERN cPropertyImpl *CreatePropertyImpl (ePropertyImpl impl, int size, cPropValueFuncs* vfunc = NULL);



/*
Local Variables:
typedefs:("BOOL" "GUID" "IComplexProperty" "IMPL" "IFACE" "IProperty" "ISimpleProperty" "LlistHead" "LlistHeader" "MetaPropertyAlistElt" "ObjID" "PropertyID" "TYPE" "cComplexProperty" "cDynArray" "cLlistPropertyImpl" "cMetaPropertyImpl" "cMetaPropertyIterator" "cProperty" "cPropertyImpl" "cPropertyManager" "cPropertyManagerKnower" "cPropertyTable" "cSimpleProperty" "cStr" "cStrHashSet" "propsElt" "tHashSetKey" "tHashSetNode")
End:
*/

#endif // PROPIMP__H
