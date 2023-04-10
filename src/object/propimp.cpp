/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propimp.cpp,v 1.14 1997/10/14 23:05:03 mahk Exp $
#include <propbase.h>
#include <propimp.h>
#include <proplist.h>
#include <prophash.h>
#include <propbig.h>
#include <propbit.h>
#include <propdb.h>
#include <propval_.h>

#include <stdlib.h>
#include <string.h>


// Must be last header
#include <dbmem.h>

uint cPropValueFuncs::Version(void)
{
   return 0;
}

////////////////////////////////////////////////////////////
//
// cDefaultPropValueFuncs
//
////////////////////////////////////////////////////////////

cDefaultPropValueFuncs::~cDefaultPropValueFuncs()
{
}

uint cDefaultPropValueFuncs::Version(void)
{
   return (uint)size_;
}

void cDefaultPropValueFuncs::Init(ObjID , void* )
{
}

void cDefaultPropValueFuncs::New(ObjID obj, void* value, void *sug)
{
   if (sug != NULL)
      Copy(obj,value,sug);
   else
      memset(value,0,size_);
}

void cDefaultPropValueFuncs::Term(ObjID, void*)
{
}

void cDefaultPropValueFuncs::Copy(ObjID, void* t, void* s)
{
   memcpy(t,s,size_);
}

void cDefaultPropValueFuncs::Read (ObjID obj, void* value, uint version, PropReadFunc read, PropFile* file)
{
   // in default, version == size
   if (version < size_)
   {
      New(obj,value);
      read(file,value,version);
   }
   else
      read(file,value,size_);
}

void cDefaultPropValueFuncs::Write(ObjID, void* value, PropWriteFunc write, PropFile* file)
{
   write(file,value,size_);
}

////////////////////////////////////////////////////////////
//
// cPropertyImpl
//
////////////////////////////////////////////////////////////


cPropertyImpl::cPropertyImpl(int size, cPropValueFuncs* f) 
   : elem_size_(size),vfunc_(f), sponsored_(TRUE)
{
   if (vfunc_ == NULL)
      vfunc_ = new cDefaultPropValueFuncs(size);
}

////////////////////////////////////////

cPropertyImpl::~cPropertyImpl()
{
   if (sponsored_)
      delete vfunc_;
}

////////////////////////////////////////

void cPropertyImpl::SetValueFuncs(cPropValueFuncs* f)
{
   if (sponsored_)
      delete vfunc_;
   vfunc_ = f;
   sponsored_ = FALSE;
}





////////////////////////////////////////////////////////////
//
// CreatePropertyImpl
//
// Creates an impl based on the ePropertyImpl value. 
//
//
cPropertyImpl *CreatePropertyImpl (ePropertyImpl impl, int size, cPropValueFuncs* f)
{
   sPropertyDatabases* db = sPropertyDatabases::DB;
   cPropertyImpl *pimpl;

   switch (impl)
   {
      case kPropertyImplLlist: 
         pimpl = new cPropertyImplLlist(size); 
         break;

      case kPropertyImplHash:
         pimpl = CreateUniHashImpl(size);
         break;
      
      case kPropertyImplSparseHash:
         pimpl = CreateMultiHashImpl(size,db->PropHash);
         break;

      case kPropertyImplBigArray:
         pimpl = CreateArrayImpl(size);
         break;

      case kPropertyImplBoolean:
         pimpl = new cPImpBitVec(size);
         break;

      //
      // Add more implementation types here
      //
      default: Warning (("CreateProperty: Bad impl %d\n", impl)); return NULL;
   }

   if (f != NULL)
      pimpl->SetValueFuncs(f);

   return pimpl;
}




/*
Local Variables:
typedefs:("BOOL" "GUID" "IComplexProperty" "IID" "INTERFACE" "IProperty" "ISimpleProperty" "LlistHeader" "ObjID" "PropertyID" "REFIID" "TYPE" "cComplexProperty" "cMetaPropertyImpl" "cMetaPropertyIterator" "cProperty" "cPropertyImpl" "cPropertyManager" "cSimpleProperty" "ePropertyImpl")
End:
*/
