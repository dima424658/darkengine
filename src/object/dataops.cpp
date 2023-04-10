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

#define min(x,y) ((x)<(y)?(x):(y))

////////////////////////////////////////////////////////////
// cDataOpsTagFile
//

STDMETHODIMP_(long) cDataOpsTagFile::Read(void* buf, int len)
{
   return File->Read((char*)buf,len); 
}   

STDMETHODIMP_(long) cDataOpsTagFile::Write(const void* buf, int len)
{
   return File->Write((char*)buf,len); 
}

STDMETHODIMP cDataOpsTagFile::Seek(eDataOpSeek seek, ulong where)
{
   static TagFileSeekMode seek_map[] = { kTagSeekFromStart, kTagSeekFromHere, kTagSeekFromEnd } ; 

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
   : mAllocatedSize(len), mUsedSize(len), mCursor(0)
{
   mBuffer = (char *)malloc(mAllocatedSize);
   memcpy(mBuffer, buf, len);
}

cDataOpsMemFile::~cDataOpsMemFile(void) 
{
   free(mBuffer);
}

STDMETHODIMP_(long)cDataOpsMemFile::Read(void* buf, int len) 
{ 
   long length = min(len, mUsedSize - mCursor);
   memcpy(buf, mBuffer + mCursor, length);
   mCursor += len;
   return length;
}

STDMETHODIMP_(long)cDataOpsMemFile::Write(const void* buf, int len)
{
   if (buf == NULL) {
      // Let's try not to crash...
      return 0;
   }
   while (mCursor + len > mAllocatedSize)
   {
      mAllocatedSize *= 2;
      char *newBuf = (char *)malloc(mAllocatedSize);
      memcpy(newBuf, mBuffer, mCursor);
      free(mBuffer);
      mBuffer = newBuf;
   }
   memcpy(mBuffer + mCursor, buf, len);
   mCursor += len;
   return len;
}

STDMETHODIMP cDataOpsMemFile::Seek(eDataOpSeek seek, ulong where)
{
   switch (seek) {
      case kDataOpSeekFromStart: mCursor = where; break;
      case kDataOpSeekFromHere: mCursor += where; break;
      case kDataOpSeekFromEnd: mCursor = mUsedSize - where; break;
   }
   if (mCursor < 0) mCursor = 0;
   if (mCursor > mUsedSize) mCursor = mUsedSize;
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
   if (!pdat->value)
      *pdat = New(); 

   char* buf = new char[version]; 
   file->Read(buf,version); 

   int size = (mSize < version) ? mSize : version; 
   memcpy(pdat->value,buf,size); 
   delete buf; 
   return S_OK; 
}

STDMETHODIMP cFixedSizeDataOps::Write(sDatum val, IDataOpsFile* file) 
{
   if (file->Write(val.value,mSize) == mSize)
      return S_OK;
   return E_FAIL; 
}


