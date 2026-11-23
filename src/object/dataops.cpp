/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/dataops.cpp,v 1.6 1999/01/07 21:45:46 JUSTIN Exp $

#include <dataops_.h>
#include <tagfile.h>
#include <string.h>

// Must be last headers
#include <dbmem.h>
#include <initguid.h>
#include <datopiid.h>

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif // !min

////////////////////////////////////////////////////////////
// cDataOpsTagFile
//

STDMETHODIMP_(long) cDataOpsTagFile::Read(void* buf, int len)
{
   if (len < 0)
      return -1;
   if (Bounded)
   {
      const ulong cursor = File->Tell();
      if (cursor < BoundStart || cursor > BoundEnd)
         return -1;
      const ulong remaining = BoundEnd - cursor;
      if ((ulong)len > remaining)
         len = (int)remaining;
   }
   return File->Read((char*)buf,len); 
}   

STDMETHODIMP_(long) cDataOpsTagFile::Write(const void* buf, int len)
{
   return File->Write((char*)buf,len); 
}

STDMETHODIMP cDataOpsTagFile::Seek(eDataOpSeek seek, ulong where)
{
   static TagFileSeekMode seek_map[] = { kTagSeekFromStart, kTagSeekFromHere, kTagSeekFromEnd } ; 

   if (seek < kDataOpSeekFromStart || seek > kDataOpSeekFromEnd)
      return E_INVALIDARG;
   if (Bounded)
   {
      ulong target;
      if (seek == kDataOpSeekFromStart)
         target = where;
      else
      {
         const ulong origin = seek == kDataOpSeekFromHere
            ? File->Tell() : BoundEnd;
         if (origin > BoundEnd || where > BoundEnd - origin)
            return E_FAIL;
         target = origin + where;
      }
      if (target < BoundStart || target > BoundEnd)
         return E_FAIL;
      return File->Seek(target,kTagSeekFromStart);
   }

   return File->Seek(where,seek_map[seek]); 
}


STDMETHODIMP_(ulong) cDataOpsTagFile::Tell()
{
   return File->Tell(); 
}

  
////////////////////////////////////////////////////////////
// cDataOpsTagFile
//
cDataOpsMemFile::cDataOpsMemFile()
   : mAllocatedSize(128), mUsedSize(0), mCursor(0)
{
   mBuffer = (char *)malloc(mAllocatedSize);
}

cDataOpsMemFile::cDataOpsMemFile(const void *buf, int len)
   : mAllocatedSize(buf && len > 0 ? len : 0), mUsedSize(0), mCursor(0)
{
   mBuffer = (char *)malloc(mAllocatedSize);
   if (mBuffer && mAllocatedSize)
   {
      memcpy(mBuffer, buf, mAllocatedSize);
      mUsedSize = mAllocatedSize;
   }
}

cDataOpsMemFile::~cDataOpsMemFile(void) 
{
   free(mBuffer);
}

STDMETHODIMP_(long)cDataOpsMemFile::Read(void* buf, int len)
{
   if (!buf || len <= 0 || mCursor >= mUsedSize)
      return 0;
   const ulong length = min((ulong)len, mUsedSize - mCursor);
   memcpy(buf, mBuffer + mCursor, length);
   mCursor += length;
   return length;
}

STDMETHODIMP_(long)cDataOpsMemFile::Write(const void* buf, int len)
{
   if (!buf || len <= 0)
      return 0;
   if ((uint)len > UINT32_MAX - mCursor)
      return 0;
   const ulong required = mCursor + (uint)len;

   if (!mBuffer || required > mAllocatedSize)
   {
      ulong capacity = mAllocatedSize ? mAllocatedSize : 128;
      while (capacity < required)
         capacity = capacity > UINT32_MAX / 2 ? required : capacity * 2;
      char *newBuffer = (char *)malloc((size_t)capacity);
      if (!newBuffer)
         return 0;
      if (mUsedSize)
         memcpy(newBuffer, mBuffer, mUsedSize);
      free(mBuffer);
      mBuffer = newBuffer;
      mAllocatedSize = capacity;
   }
   memcpy(mBuffer + mCursor, buf, len);
   mCursor += len;
   if (mCursor > mUsedSize)
      mUsedSize = mCursor;
   return len;
}

STDMETHODIMP cDataOpsMemFile::Seek(eDataOpSeek seek, ulong where)
{
   ulong cursor;
   switch (seek)
   {
      case kDataOpSeekFromStart:
         cursor = where;
         break;
      case kDataOpSeekFromHere:
         cursor = where > mUsedSize - mCursor
            ? mUsedSize : mCursor + where;
         break;
      case kDataOpSeekFromEnd:
         cursor = where > mUsedSize ? 0 : mUsedSize - where;
         break;
      default:
         return E_INVALIDARG;
   }
   mCursor = min(cursor, mUsedSize);
   return S_OK;
}

STDMETHODIMP_(ulong)cDataOpsMemFile::Tell() 
{ 
   return mCursor;
}

STDMETHODIMP_(void *)cDataOpsMemFile::GetBuffer()
{
   return mBuffer;
}


////////////////////////////////////////////////////////////
// cFixedSizeDataOps
//

STDMETHODIMP_(sDatum) cFixedSizeDataOps::New() 
{ 
   sDatum dat(new char[mSize]); 
   memset(dat.value,0,mSize);
   return dat; 
} 

STDMETHODIMP_(sDatum) cFixedSizeDataOps::CopyNew(sDatum val) 
{ 
   sDatum newval = New(); 
   Copy(&newval,val); 
   return newval; 
} 

STDMETHODIMP cFixedSizeDataOps::Delete(sDatum val) 
{ 
   delete [] (char*)val.value; 
   return S_OK; 
}

STDMETHODIMP cFixedSizeDataOps::Copy(sDatum* targ, sDatum src)
{
   if (targ->value != src.value)
      memcpy(targ->value,src.value,mSize); 
   return S_OK; 
}

STDMETHODIMP_(long) cFixedSizeDataOps::BlockSize(sDatum )
{ 
   return mSize; 
}

STDMETHODIMP_(int) cFixedSizeDataOps::Version() 
{ 
   return mSize; 
} 

STDMETHODIMP cFixedSizeDataOps::Read(sDatum* pdat, IDataOpsFile* file, int version ) 
{
   // A version is a serialized byte count, not a trusted allocation size.
   if (version < 0 || version > 64 * 1024 * 1024)
      return E_FAIL;

   char *buf = new (std::nothrow) char[version];
   if (!buf)
      return E_OUTOFMEMORY;
   if (file->Read(buf, version) != version)
   {
      delete [] buf;
      return E_FAIL;
   }

   if (!pdat->value)
      *pdat = New();
   const int size = (mSize < version) ? mSize : version;
   memcpy(pdat->value, buf, size);
   delete [] buf;
   return S_OK;
}

STDMETHODIMP cFixedSizeDataOps::Write(sDatum val, IDataOpsFile* file) 
{
   if (file->Write(val.value,mSize) == mSize)
      return S_OK;
   return E_FAIL; 
}


