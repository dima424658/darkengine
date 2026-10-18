/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/buftagf.cpp,v 2.2 2000/03/07 19:56:48 toml Exp $
#include <stdio.h>
#include <platform_io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <cfgdbg.h>
#include <platform_services.h>
#include <dataguard.h>

#include <lg.h>
#include <vernum.h>
#include <buftagf.h>

#include <config.h>
#include <cfgdbg.h>

// implement hash sets
#include <hshsttem.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Yet another memory file! No write support, very specialized, open is very very expensive
//

#define kMinSysMem (64*1024*1024)
#define kMinSize (256*1024)

struct VFILE
{
   FILE * file;

   BYTE * pBuf;
   BYTE * pReadLoc;
   BYTE * pLimit;
   uint64_t length;
};

///////////////////////////////////////

VFILE * vfopen(const char * file, const char * mode)
{
   FILE * f;
   
   if ((f = fopen(file, mode)) == NULL)
      return NULL;
   
   VFILE * pVFile = new VFILE;

   memset(pVFile,0, sizeof(*pVFile));

   pVFile->file = f;
   if (DarkFileSeek64(f,0,SEEK_END) != 0)
   {
      fclose(f);
      delete pVFile;
      return NULL;
   }
   int64_t physicalSize = DarkFileTell64(f);
   if (physicalSize < 0 || DarkFileSeek64(f,0,SEEK_SET) != 0)
   {
      fclose(f);
      delete pVFile;
      return NULL;
   }
   pVFile->length = (uint64_t)physicalSize;

   static int fHaveMem = -1;
   if (fHaveMem == -1)
   {
      sPlatformMemoryStatus memoryStatus;
      PlatformGetMemoryStatus(&memoryStatus);
      
      fHaveMem = (memoryStatus.total_physical_bytes >= kMinSysMem);
   }
      
   if (fHaveMem && *mode == 'r' && config_is_defined("large_read_buffers"))
   {
      size_t bufferedSize = 0;
      if (pVFile->length > kMinSize &&
          DarkCheckedSizeT(pVFile->length,&bufferedSize))
      {
         pVFile->pBuf = (BYTE *)malloc(bufferedSize);
         if (pVFile->pBuf)
         {
            size_t got = fread(pVFile->pBuf,1,bufferedSize,pVFile->file);
            if (got != bufferedSize)
            {
               free(pVFile->pBuf);
               pVFile->pBuf = NULL;
            }
            pVFile->pReadLoc = pVFile->pBuf;
            pVFile->pLimit = pVFile->pBuf ? pVFile->pBuf + bufferedSize : NULL;
         }
      }
   }
   
   return pVFile;
}

///////////////////////////////////////

size_t vfread(void * pTo, size_t s, size_t n, VFILE * pVFile)
{
   if (pVFile->pBuf)
   {
      uint64_t requested = 0;
      size_t sz = 0;
      if (!DarkCheckedMulU64(s,n,&requested) ||
          !DarkCheckedSizeT(requested,&sz) || s == 0)
         return 0;
      
      if (pVFile->pReadLoc >= pVFile->pLimit)
         return 0;
      size_t available = (size_t)(pVFile->pLimit - pVFile->pReadLoc);
      if (sz > available)
         sz = available;
         
      memcpy(pTo, pVFile->pReadLoc, sz);
      pVFile->pReadLoc += sz;
      
      return sz / s;
   }
   
   return fread(pTo, s, n, pVFile->file);
}

///////////////////////////////////////

size_t vfwrite(const void * pFrom, size_t s, size_t n, VFILE * pVFile)
{
   Assert_(!pVFile->pBuf);
   return fwrite(pFrom, s, n, pVFile->file);
}

///////////////////////////////////////

static int vfseek64(VFILE * pVFile, int64_t offset, int type)
{
   if (pVFile->pBuf)
   {
      int64_t base = 0;
      switch (type)
      {
         case SEEK_SET: base = 0; break;
         case SEEK_CUR: base = pVFile->pReadLoc - pVFile->pBuf; break;
         case SEEK_END: base = (int64_t)pVFile->length; break;
         default: return -1;
      }
      if ((offset < 0 && (offset == INT64_MIN || base < -offset)) ||
          (offset > 0 && base > INT64_MAX - offset))
         return -1;
      int64_t target = base + offset;
      if (target < 0 || (uint64_t)target > pVFile->length)
         return -1;
      pVFile->pReadLoc = pVFile->pBuf + (size_t)target;
      return 0;
   }

   return DarkFileSeek64(pVFile->file,offset,type);
}

int vfseek(VFILE * pVFile, long offset, int type)
{
   return vfseek64(pVFile,(int64_t)offset,type);
}

///////////////////////////////////////

long vftell(VFILE * pVFile)
{
   if (pVFile->pBuf)
   return (long)(pVFile->pReadLoc - pVFile->pBuf);
   
   int64_t result = DarkFileTell64(pVFile->file);
   return result >= 0 && result <= LONG_MAX ? (long)result : -1;
}

static int64_t vftell64(VFILE *pVFile)
{
   if (pVFile->pBuf)
      return (int64_t)(pVFile->pReadLoc - pVFile->pBuf);
   return DarkFileTell64(pVFile->file);
}

///////////////////////////////////////

int vfclose(VFILE * pVFile)
{
   if (!pVFile)
      return -1;
   
   int result = fclose(pVFile->file);

   if (pVFile->pBuf)
   {
      free(pVFile->pBuf);
   }

   delete pVFile;
   return result;
}

///////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
// TagFileVersion
// 

static const TagVersion MyVersion = 
{
  0,
  1,
}; 

static uchar Deadbeef[] = { 0xDE, 0xAD, 0xBE, 0xEF};

////////////////////////////////////////////////////////////
// TAG TABLE IMPLEMENTATION
//



tHashSetKey BufTagFileTable::GetKey(tHashSetNode node) const
{
   BufTagTableEntry* e = (BufTagTableEntry*)node;
   return (tHashSetKey)(&e->key);
}

////////////////////////////////////////

#define WRITE(file,x) Verify(vfwrite(&(x),sizeof(x),1,file) == 1)

void BufTagFileTable::Write(VFILE* file)
{
   tHashSetHandle idx;
   const BufTagTableEntry* e;
   ulong items = GetCount(); 
   WRITE(file,items);
   for (e = GetFirst(idx); e != NULL; e = GetNext(idx))
   {
      WRITE(file,*e);
   }
}

////////////////////////////////////////

BufTagFileTable::~BufTagFileTable()
{
   tHashSetHandle handle;
   BufTagTableEntry* e;

   for (e = GetFirst(handle); e != NULL; e = GetNext(handle))
   {
      delete e;
   }
}

////////////////////////////////////////

template <typename T>
static bool vfread_exact(VFILE *file, T& value)
{
   return vfread(&value,sizeof(value),1,file) == 1;
}

bool BufTagFileTable::Read(VFILE* file, uint64_t fileSize,
                           uint64_t tableOffset)
{
   ulong items = 0;
   if (!vfread_exact(file,items))
      return false;
   uint64_t directoryStart = 0;
   uint64_t directoryBytes = 0;
   if (!DarkCheckedAddU64(tableOffset,sizeof(items),&directoryStart) ||
       items > 1024U * 1024U ||
       !DarkCheckedMulU64(items,sizeof(BufTagTableEntry),&directoryBytes) ||
       !DarkRangeWithinU64(directoryStart,directoryBytes,fileSize))
      return false;

   for (ulong i = 0; i < items; i++)
   {
      BufTagTableEntry e;
      if (!vfread_exact(file,e) ||
          !DarkBufferHasNull(e.key.label,sizeof(e.key.label)) ||
          e.offset < sizeof(BufTagFileHeader) ||
          !DarkRangeWithinU64(e.offset,sizeof(BufTagFileBlockHeader),tableOffset) ||
          !DarkRangeWithinU64(e.Start(),e.size,tableOffset) ||
          Search(&e.key) != NULL)
         return false;

      int64_t directoryPosition = vftell64(file);
      BufTagFileBlockHeader header;
      if (directoryPosition < 0 || vfseek64(file,e.offset,SEEK_SET) != 0 ||
          !vfread_exact(file,header) ||
          !DarkBufferHasNull(header.tag.label,sizeof(header.tag.label)) ||
          memcmp(&header.tag,&e.key,sizeof(e.key)) != 0 ||
          vfseek64(file,directoryPosition,SEEK_SET) != 0)
         return false;

      BufTagTableEntry *entry = new BufTagTableEntry(e);
      if (!Insert(entry))
      {
         delete entry;
         return false;
      }
   }
   return true;
}

////////////////////////////////////////////////////////////
// BufTagFileBase implementation
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(BufTagFileBase,ITagFile);


BufTagFileBase::BufTagFileBase(const char* fname, const char* mode)
 : file(vfopen(fname,mode)),block(NULL),blockVersion{0,0},fileSize(0) {} ;

////////////////////////////////////////

BufTagFileBase::~BufTagFileBase()
{
   if (block != NULL)
   {
      Warning(("Buffered tag file closing with block %s still open\n",
               block->key.label));
      SetCurBlock(NULL);
   }
   if (file != NULL)
      vfclose(file);
   file = NULL;
}


////////////////////////////////////////

STDMETHODIMP_(const TagVersion*) BufTagFileBase::GetVersion()
{
   return &MyVersion;
}

////////////////////////////////////////

STDMETHODIMP_(ulong) BufTagFileBase::BlockSize(const TagFileTag* tag)
{
   BufTagTableEntry* e = table.Search(tag); 
   if (e == NULL)
      return kTagNoSuchBlock;
   return e->size;
}

////////////////////////////////////////

STDMETHODIMP_(const TagFileTag*) BufTagFileBase::CurrentBlock()
{
   if (block == NULL)
      return NULL;   
   return &block->key;
}

////////////////////////////////////////

void BufTagFileBase::SetCurBlock(const TagFileTag* tag)
{
   if (tag == NULL)
      block = NULL;
   else
      block = table.Search(tag);
}

////////////////////////////////////////

STDMETHODIMP BufTagFileBase::Seek(ulong offset, TagFileSeekMode mode)
{
   if (block == NULL)
   {
      Warning(("Seeking when no block is open\n"));
      return E_FAIL;
   }

   uint64_t base = 0;
   switch (mode)
   {
      case kTagSeekFromStart: base = block->Start(); break;
      case kTagSeekFromHere:
      {
         int64_t current = vftell64(file);
         if (current < 0) return E_FAIL;
         base = (uint64_t)current;
         break;
      }
      case kTagSeekFromEnd: base = block->End(); break;
      default: return E_FAIL;
   }
   uint64_t absolute = 0;
   if (!DarkCheckedAddU64(base,offset,&absolute) ||
       absolute < block->Start() ||
       (OpenMode() == kTagOpenRead && absolute > block->End()) ||
       absolute > INT64_MAX)
   {
      Warning(("Seeking outside of tag file block\n"));
      return E_FAIL;
   }
   if (vfseek64(file,(int64_t)absolute,SEEK_SET) != 0)
      return E_FAIL;
   blockptr = (ulong)(absolute - block->Start());

   if (OpenMode() == kTagOpenWrite)
   {
      if (blockptr > block->size)
         block->size = blockptr;
   }

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(ulong) BufTagFileBase::Tell()
{
   if (block == NULL)
      return 0; 
   return blockptr;
}

////////////////////////////////////////

STDMETHODIMP_(ulong) BufTagFileBase::TellFromEnd()
{
   if (block == NULL)
      return 0; 
   return blockptr <= block->size ? block->size - blockptr : 0;
}

//------------------------------------------------------------
// Iteration
//

class cBufTagIter : public ITagFileIter 
{
   tHashSetHandle Idx;
   BufTagFileTable& Table;
   const BufTagTableEntry* Entry;

public:

   cBufTagIter(BufTagFileTable& t)
      :Entry(NULL),Table(t)
   {
   }

   virtual ~cBufTagIter() {}; 

   DECLARE_UNAGGREGATABLE(); 

   STDMETHOD(Start)() { Entry = Table.GetFirst(Idx); return S_OK; }; 
   STDMETHOD_(BOOL,Done)() { return Entry == NULL; }; 
   STDMETHOD(Next)() { Entry = Table.GetNext(Idx); return S_OK; }; 

   STDMETHOD_(const TagFileTag*,Tag)() { return (Entry) ? &Entry->key : NULL; } ; 
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBufTagIter,ITagFileIter);

STDMETHODIMP_(ITagFileIter*) BufTagFileBase::Iterate()
{
   return new cBufTagIter(table); 
}

////////////////////////////////////////////////////////////
// BufTagFileWrite implementation
//

STDMETHODIMP_(TagFileOpenMode) BufTagFileWrite::OpenMode()
{
   return kTagOpenWrite;
}

////////////////////////////////////////

BufTagFileWrite::BufTagFileWrite(const char* filename)
   : BufTagFileBase(filename,"wb")
{
   if (file != NULL)
   {
      // write a place holder for header
      BufTagFileHeader header;
      WRITE(file,header);
   }
   else
   {
      Warning(("BufTagFileWrite: opening %s for writing failed\n",filename));
   }
}

////////////////////////////////////////

BufTagFileWrite::~BufTagFileWrite()
{
   if (file != NULL)
   {
      BufTagFileHeader header;

      vfseek(file,0,SEEK_END); // seek to end 
      header.table = vftell(file);

      table.Write(file); // write out the table

      vfseek(file,0,SEEK_SET); // seek to front

      // fill out header
      header.version = MyVersion;
      memcpy(header.deadbeef,Deadbeef,sizeof(header.deadbeef));

      // write out actual data in header
      WRITE(file,header);
   }
}


////////////////////////////////////////

STDMETHODIMP BufTagFileWrite::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
   
   if (file == NULL)
      return E_FAIL;

   if (block != NULL)
   {
      Warning(("Opening Tag Block with a block already open\n"));
      return E_FAIL;
   }

   AssertMsg1(table.Search(tag) == NULL,"BufTagFileWrite::NewBlock(): tag %s is already in use",tag->label);

   BufTagFileBlockHeader header;

   header.tag = *tag;
   header.version = *version;

   // put the block at the end of the file!
   vfseek(file,0,SEEK_END);

   // store offset and tag in table
   BufTagTableEntry* entry = new BufTagTableEntry(*tag,vftell(file));
   void* result = table.Insert(entry);
   if (!result) return E_FAIL;

   SetCurBlock(tag);

   int len = (int)vfwrite(&header,1,sizeof(header),file);
   if (len != sizeof(header))
   {
      Warning(("BufTagFileWrite::NewBlock(): wrote only %d out of %d bytes\n",len,sizeof(header)));
      return E_FAIL;
   }

   blockptr = 0;
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP BufTagFileWrite::CloseBlock()
{
   if (block == NULL)
   {
      Warning(("BufTagFileWrite::CloseBlock() No Block to Close\n"));
      return E_FAIL;
   }
   SetCurBlock(NULL);
   return S_OK;
}

////////////////////////////////////////




////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileWrite::Read(char* , int )
{
   Warning(("Reading a TagFile opened for writing\n"));
   return -1;
}



////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileWrite::Write(const char* buf, int buflen)
{
   AssertMsg(block != NULL, "BufTagFileWrite::Write(): No block has been started\n");
   if (block == NULL) return -1;
   
   if (file == NULL)
      return -1;

   long len = (long)vfwrite(buf,1,buflen,file);
   blockptr += len; 
   
   if (blockptr > block->size)
      block->size = blockptr;

   return len;
}

////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileWrite::Move(char* buf, int len)
{
   return Write(buf,len);
}

////////////////////////////////////////////////////////////
// BufTagFileRead Implementation 
//

STDMETHODIMP_(TagFileOpenMode) BufTagFileRead::OpenMode()
{
   return kTagOpenRead;
}

////////////////////////////////////////

BufTagFileRead::BufTagFileRead(const char* filename)
   : BufTagFileBase(filename,"rb")
{
   if (file == NULL)
   {
      ConfigSpew("TagFileTrace",("BufTagFileRead: opening %s for reading failed\n",filename));
      return;
   }
   fileSize = file->length;
   if (fileSize < sizeof(BufTagFileHeader) ||
       vfseek64(file,0,SEEK_SET) != 0)
   {
      TagFileNotifyError(this,NULL,"header",
                         "file is shorter than the tag-file header");
      vfclose(file); file = NULL; return;
   }

   // read in header
   BufTagFileHeader header;
   if (!vfread_exact(file,header))
   {
      TagFileNotifyError(this,NULL,"header","truncated tag-file header");
      vfclose(file); file = NULL; return;
   }

   // verify header.

   if (memcmp(header.deadbeef,Deadbeef,sizeof(header.deadbeef)) != 0)
   {
      Warning(("Tag file %s is corrupt!\n",filename));
      TagFileNotifyError(this,NULL,"header","invalid tag-file signature");
      vfclose (file);
      file = NULL;
      return; 
   }

   // warn about version
   int delta = VersionNumsCompare(&MyVersion,&header.version);
   if (delta)
   {
#ifdef DBG_ON
      char buf[256];
      strcpy(buf,VersionNum2String(&MyVersion));
      Warning(("Tag file %s is not current version.\nOld: %s, New: %s\n",
               filename,VersionNum2String(&header.version),buf));
#endif 
      // Do not discard a newer container. Individual block codecs decide
      // compatibility after the complete directory has been inventoried.
   }


   // Read and validate the complete directory before any codec sees it.
   if (header.table < sizeof(BufTagFileHeader) ||
       !DarkRangeWithinU64(header.table,sizeof(ulong),fileSize) ||
       vfseek64(file,header.table,SEEK_SET) != 0 ||
       !table.Read(file,fileSize,header.table))
   {
      TagFileNotifyError(this,NULL,"directory",
         "invalid count, duplicate tag, block range, or block header");
      vfclose(file); file = NULL; return;
   }
   
}

////////////////////////////////////////

BufTagFileRead::~BufTagFileRead()
{
      
}

////////////////////////////////////////

STDMETHODIMP BufTagFileRead::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
   if (file == NULL)
      return E_FAIL;
   
   SetCurBlock(tag);

   if (block == NULL)
   {
      ConfigSpew("tagfile_spew", ("Tag %s not in tag file\n", tag->label));
      return E_FAIL;
   }
   if (vfseek64(file,block->offset,SEEK_SET) != 0)
   {
      TagFileNotifyError(this,tag,"open block","block offset is not seekable");
      SetCurBlock(NULL);
      return E_FAIL;
   }

   BufTagFileBlockHeader header;
   int len = (int)vfread(&header,1,sizeof(header),file);
   if (len != sizeof(header))
   { 
      Warning(("BufTagFileRead::OpenBlock(): read only %d of %d bytes\n",len,sizeof(header)));
      TagFileNotifyError(this,tag,"open block","truncated block header");
      SetCurBlock(NULL);
      return E_FAIL;
   }
   if (!DarkBufferHasNull(header.tag.label,sizeof(header.tag.label)) ||
       memcmp(&header.tag,&block->key,sizeof(header.tag)) != 0)
   {
      TagFileNotifyError(this,tag,"open block",
                         "block header disagrees with directory");
      SetCurBlock(NULL);
      return E_FAIL;
   }
   
   // The caller supplies its read ceiling.  A wildcard ceiling is used only
   // by inventory code; normal codecs remain fail-closed for newer blocks.
   int delta = VersionNumsCompare(version,&header.version);
   if (delta != 0)
   {
#ifdef DBG_ON
      char buf[256];
      strcpy(buf,VersionNum2String(version));
      ConfigSpew("tagfile_spew",("Tag data %s is not current version.\nOld: %s, New: %s\n",
               tag->label,VersionNum2String(&header.version),buf));
#endif 
      if (delta < 0)
      {
         ConfigSpew("tagfile_spew",("Version in file is newer than the codec read ceiling.\n"));
         SetCurBlock(NULL);
         return E_FAIL;
      }
   }
   
   *version = header.version;
   blockVersion = header.version;
   blockptr = 0;
   TagFileNotifyBlockObserver(this, &block->key, &blockVersion,
                              block->size, 0, FALSE);

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP BufTagFileRead::CloseBlock()
{
   if (block == NULL)
   {
      Warning(("BufTagFileRead::CloseBlock() No Block to Close\n"));
      return E_FAIL;
   }
   TagFileNotifyBlockObserver(this, &block->key, &blockVersion,
                              block->size, blockptr, TRUE);
   SetCurBlock(NULL);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileRead::Read(char* buf, int buflen)
{
   if (file == NULL)
      return -1;

   if (block == NULL)
   {
      Warning(("BufTagFileRead::Read() called before ::Seek()\n"));
      return -1;
   }

   if (buflen < 0)
      return -1;
   if (blockptr > block->size)
   {
      TagFileNotifyError(this,&block->key,"read",
                         "block cursor is outside the validated range");
      return -1;
   }
   ulong bytesleft = block->size - blockptr;
   if ((ulong)buflen > bytesleft)
      buflen = (int)bytesleft;
   
   int len = (int)vfread(buf,1,buflen,file);
   blockptr += len;
   if (len != buflen)
      TagFileNotifyError(this,&block->key,"read",
                         "physical file ended inside a block");
   return len;
}

////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileRead::Write(const char* , int )
{
   Warning(("Writing to a tagfile opened for reading\n"));
   return -1;
}

////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileRead::Move(char* buf, int buflen)
{
   return Read(buf,buflen);
}


//////////////////////////////////////////////////////////////
// TagFileOpen
// 

ITagFile* BufTagFileBase::Open(const char* filename, TagFileOpenMode mode)
{
   BufTagFileBase* result = NULL;
   switch(mode)
   {
      case kTagOpenRead:
         result = new BufTagFileRead(filename);
         break;
      case kTagOpenWrite:
         result = new BufTagFileWrite(filename);
         break;
      default:
         return NULL;
   }
   if (result->file == NULL)
   {
      delete result;
      result = NULL;
   }
   return result;
}


ITagFile* BufTagFileOpen(const char* filename, TagFileOpenMode mode)
{
   return BufTagFileBase::Open(filename,mode);
}
