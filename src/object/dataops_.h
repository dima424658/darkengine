/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/dataops_.h,v 1.7 2000/02/21 12:15:07 toml Exp $
#pragma once

#ifndef __DATAOPS__H
#define __DATAOPS__H

#include <dataops.h>
#include <comtools.h>
#include <string.h>
#include <lgassert.h>

#include <memall.h>
#include <dbmem.h>

////////////////////////////////////////////////////////////
// COMMON IMPLEMENTATION CLASSES FOR DATA OPS
//

//
// Base COM methods
//

class cBaseDataOps : public cCTUnaggregated<IDataOps,&IID_IDataOps, kCTU_NoSelfDelete>
{
public:
   // self-deletion is the norm, just not mandatory
   void OnFinalRelease() { delete this; };
};

//------------------------------------------------------------
// Use the constructors/deconstructors of a class
//


template <class TYPE>
class cClassDataOps : public cBaseDataOps
{


   // we don't self-delete
   void OnFinalRelease() {  };

public:

   enum eFlags
   {
      kNoFlags = 0,
      kZeroMemory = 1 << 0,  // zero on init
   };

   // dopey constructor
   cClassDataOps(eFlags flags = kZeroMemory)
      : mFlags(flags) {};

   STDMETHOD_(sDatum,New)()
   {
      TYPE* result = new TYPE;
      if (mFlags & kZeroMemory) memset(result,0,sizeof(*result));
      return result;
   };

   STDMETHOD_(sDatum,CopyNew)(sDatum val)
   { return new TYPE(*(TYPE*)val.value); };

   STDMETHOD(Delete)(sDatum val) { delete (TYPE*)val.value; return S_OK; };

   STDMETHOD(Copy)(sDatum* targ, sDatum src)
   {
      if (targ->value != src.value)
         *(TYPE*)targ->value = *(TYPE*)src.value;
      return S_OK;
   }

   STDMETHOD_(long,BlockSize)(sDatum )
   { return sizeof(TYPE); };

   STDMETHOD_(int,Version)() { return sizeof(TYPE); };

   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version )
   {
      // assumes version is the size
      char* buf = new char[version];
      file->Read(buf,version);

      if (!pdat->value)
         *pdat = New();

      if (version != sizeof(TYPE))
      {
         int size = (sizeof(TYPE) < version) ? sizeof(TYPE) : version;
         memcpy(pdat->value,buf,size);
      }
      else
         Copy(pdat,sDatum(buf));

      delete buf;
      return S_OK;
   }

   STDMETHOD(Write)(sDatum val, IDataOpsFile* file)
   {
      if (file->Write((void*)val,sizeof(TYPE)) == sizeof(TYPE))
         return S_OK;
      return E_FAIL;
   }

   const eFlags mFlags;
};

//------------------------------------------------------------
// Memory blocks of fixed size
//

class cFixedSizeDataOps : public cBaseDataOps
{
   int mSize;

public:
   cFixedSizeDataOps(int size) : mSize(size) {};

   STDMETHOD_(sDatum,New)();
   STDMETHOD_(sDatum,CopyNew)(sDatum);
   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version);
   STDMETHOD(Delete)(sDatum val);
   STDMETHOD(Copy)(sDatum* targ, sDatum src);
   STDMETHOD_(long, BlockSize)(sDatum val);
   STDMETHOD_(int,Version)();
   STDMETHOD(Write)(sDatum, IDataOpsFile* file);
};

//------------------------------------------------------------
// Data ops for integer-like things
//

class cSimpleDataOps : public cBaseDataOps
{
   // we don't self-delete
   void OnFinalRelease() { };

public:
   STDMETHOD_(sDatum,New)() { return 0; };
   STDMETHOD_(sDatum,CopyNew)(sDatum val) { return val; };

   STDMETHOD(Delete)(sDatum ) { return S_OK; };

   STDMETHOD(Copy)(sDatum* targ, sDatum src) { *targ = src; return S_OK; };

   STDMETHOD_(long,BlockSize)(sDatum)
   { return -1; /* not a pointer */ };

   STDMETHOD_(int,Version)() { return 4; };

   STDMETHOD(Read)(sDatum* pval, IDataOpsFile* file, int )
   {
      if (file->Read((void*)pval,sizeof(*pval)) == sizeof(*pval))
         return S_OK;
      return E_FAIL;
   }

   STDMETHOD(Write)(sDatum val, IDataOpsFile* file)
   {
      if (file->Write((void*)&val,sizeof(val)) == sizeof(val))
         return S_OK;
      return E_FAIL;
   }
};

typedef cSimpleDataOps cIntDataOps;

//------------------------------------------------------------
// Delegating data ops proxy.
//

class cDelegatingDataOps : public cCTDelegating<IDataOps>
{
   IDataOps* mpOps;
public:
   cDelegatingDataOps(IDataOps* inner = NULL, IUnknown* outer = NULL)
      : cCTDelegating<IDataOps>(outer),
        mpOps(inner)
   {
      if (mpOps)
         mpOps->AddRef();
   }

   ~cDelegatingDataOps() { SafeRelease(mpOps); };

   //
   // Useful non-method
   //
   void SetOps(IDataOps* ops)
   {
      Assert_(ops);
      ops->AddRef();
      SafeRelease(mpOps);
      mpOps = ops;
   };

   //
   // Methods
   //

   STDMETHOD_(sDatum,New)() { Assert_(mpOps); return mpOps->New(); };

   STDMETHOD_(sDatum,CopyNew)(sDatum dat) { Assert_(mpOps); return mpOps->CopyNew(dat); };
   STDMETHOD(Delete)(sDatum val) { Assert_(mpOps); return mpOps->Delete(val); };
   STDMETHOD(Copy)(sDatum* targ, sDatum src) { Assert_(mpOps); return mpOps->Copy(targ,src); };
   STDMETHOD_(long, BlockSize)(sDatum val) { Assert_(mpOps); return mpOps->BlockSize(val); };
   STDMETHOD_(int,Version)() { Assert_(mpOps); return mpOps->Version(); };
   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version) { Assert_(mpOps); return mpOps->Read(dat,file,version); };
   STDMETHOD(Write)(sDatum dat, IDataOpsFile* file) { Assert_(mpOps); return mpOps->Write(dat,file); };


};

//------------------------------------------------------------
// ITagFile reader-writers
//

F_DECLARE_INTERFACE(ITagFile);

class cDataOpsTagFile : public cCTUnaggregated<IDataOpsFile,&IID_IDataOpsFile,kCTU_Default>
{
   ITagFile* File;
public:
   cDataOpsTagFile(ITagFile* file)
      : File(file)
   {}

   STDMETHOD_(long,Read)(void* buf, int len);
   STDMETHOD_(long,Write)(const void* buf, int len);

   STDMETHOD(Seek)(eDataOpSeek seek, ulong where);
   STDMETHOD_(ulong,Tell)();


};

F_DECLARE_INTERFACE(IDataOpsMemFile);

// cDataOpsMemFile is a version of IDataOpsFile that writes to a memory buffer rather
// than writing to a file.
class cDataOpsMemFile : public cCTUnaggregated<IDataOpsMemFile,&IID_IDataOpsMemFile,kCTU_Default>
{
   char *mBuffer;  // The buffer where we write & read.
   ulong mAllocatedSize;
   ulong mUsedSize;
   ulong mCursor;  // The current position returned by Tell()
public:
   cDataOpsMemFile(void);
   cDataOpsMemFile(const void *buf, int len);
   ~cDataOpsMemFile(void);

   // Methods from IDataOpsFile
   STDMETHOD_(long,Read)(void* buf, int len);
   STDMETHOD_(long,Write)(const void* buf, int len);
   STDMETHOD(Seek)(eDataOpSeek seek, ulong where);
   STDMETHOD_(ulong,Tell)();

   // IDataOpsMemFile method
   STDMETHOD_(void *,GetBuffer)(THIS);
};

#include <undbmem.h>

#endif // __DATAOPS__H



