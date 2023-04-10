/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/buftagf.cpp,v 2.2 2000/03/07 19:56:48 toml Exp $
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>
#include <cfgdbg.h>

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

   static int fHaveMem = -1;
   if (fHaveMem == -1)
   {
      MEMORYSTATUS memoryStatus;
      memoryStatus.dwLength = sizeof(memoryStatus);
      GlobalMemoryStatus(&memoryStatus);
      
      fHaveMem = (memoryStatus.dwTotalPhys >= kMinSysMem);
   }
      
   if (fHaveMem && *mode == 'r' && config_is_defined("large_read_buffers"))
   {
      struct _stat s;
      if (_fstat(_fileno(pVFile->file), &s) != -1)
      {
         if (s.st_size > kMinSize)
         {
#ifdef DEBUG_BUFTAGF
            pVFile->pBuf = (BYTE *)malloc(s.st_size);
#else
            pVFile->pBuf = (BYTE *)VirtualAlloc(NULL, s.st_size, MEM_COMMIT, PAGE_READWRITE);
#endif
            if (pVFile->pBuf)
            {
               fread(pVFile->pBuf, s.st_size, 1, pVFile->file);
               if (ferror(pVFile->file) != 0)
               {
#ifdef DEBUG_BUFTAGF
                  free(pVFile->pBuf);
#else
                  VirtualFree(pVFile->pBuf, 0, MEM_RELEASE);
#endif
                  pVFile->pBuf = NULL;
               }
               pVFile->pReadLoc = pVFile->pBuf;
               pVFile->pLimit = pVFile->pBuf + s.st_size;
            }
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
      int sz = s * n;
      
      if (pVFile->pReadLoc >= pVFile->pLimit)
         return 0;
      if (pVFile->pReadLoc + sz > pVFile->pLimit)
         sz -= (pVFile->pReadLoc + sz) - pVFile->pLimit;
         
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

int vfseek(VFILE * pVFile, long offset, int type)
{
   if (pVFile->pBuf)
   {
      switch (type)
      {
         case SEEK_SET:
         {
            pVFile->pReadLoc = pVFile->pBuf;
            // fall through...
         }
         case SEEK_CUR:
         {
            pVFile->pReadLoc += offset;
            break;
         }
         case SEEK_END:
         {
            Assert_(offset == 0);
            pVFile->pReadLoc = pVFile->pLimit;
            break;
         }
      }
      return 0;
   }

   return fseek(pVFile->file, offset, type);
}

///////////////////////////////////////

long vftell(VFILE * pVFile)
{
   if (pVFile->pBuf)
      return pVFile->pReadLoc - pVFile->pBuf;
   
   return ftell(pVFile->file);
}

///////////////////////////////////////

int vfclose(VFILE * pVFile)
{
   if (!pVFile)
      return -1;
   
   int result = fclose(pVFile->file);

   if (pVFile->pBuf)
   {
#ifdef DEBUG_BUFTAGF
      free(pVFile->pBuf);
#else
      VirtualFree(pVFile->pBuf, 0, MEM_RELEASE);
#endif
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

#define READ(file,x) Verify(vfread(&(x),sizeof(x),1,file) == 1)

void BufTagFileTable::Read(VFILE* file)
{
   ulong items;
   READ(file,items);
   for (int i = 0; i < items; i++)
   {
      BufTagTableEntry e;
      READ(file,e);
      Insert(new BufTagTableEntry(e));
   }
}

////////////////////////////////////////////////////////////
// BufTagFileBase implementation
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(BufTagFileBase,ITagFile);


BufTagFileBase::BufTagFileBase(const char* fname, const char* mode)
 : file(vfopen(fname,mode)),block(NULL) {} ; 

////////////////////////////////////////

BufTagFileBase::~BufTagFileBase()
{
   AssertMsg(block == NULL,"Tag file closing with a block still open");
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

int BufTagFileBase::PrepSeek(ulong& offset, TagFileSeekMode mode)
{
   if (block == NULL)
      return FALSE;

   // convert offset to absolute
   switch (mode)
   {
      case kTagSeekFromStart:
         offset += block->Start();
         break;
      case kTagSeekFromHere:
         offset += vftell(file);
         break;
      case kTagSeekFromEnd:
         offset += block->End();
         break;
   }

   if (offset < block->Start()) 
      return block->offset - block->Start();
   if (offset > block->End())
      return offset - block->End();

   return 0;
}

////////////////////////////////////////

#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))


STDMETHODIMP BufTagFileBase::Seek(ulong offset, TagFileSeekMode mode)
{
   if (block == NULL)
   {
      Warning(("Seeking when no block is open\n"));
      return E_FAIL;
   }

   int check = PrepSeek(offset,mode);

   // if out of bounds, fail.
   if (check < 0 || (check > 0 && OpenMode() == kTagOpenRead)) 
   {
      Warning(("Seeking outside of tag file block\n"));
      return E_FAIL;
   }
   // @TBD: change logic so that SEEK_CUR is preserved for performance
   vfseek(file,offset,SEEK_SET);
   blockptr = offset - block->Start();

   if (OpenMode() == kTagOpenWrite)
   {
      block->size = max(block->size,blockptr);
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
   return block->size - blockptr;
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

   int len = vfwrite(&header,1,sizeof(header),file);
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

   long len = vfwrite(buf,1,buflen,file);
   blockptr += len; 
   
   block->size = max(blockptr,block->size);

   return len;
}

////////////////////////////////////////

STDMETHODIMP BufTagFileWrite::Move(char* buf , int len)
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
   // read in header
   BufTagFileHeader header;
   READ(file,header);

   // verify header.

   if (memcmp(header.deadbeef,Deadbeef,sizeof(header.deadbeef)) != 0)
   {
      Warning(("Tag file %s is corrupt!\n",filename));
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
      // if it's NEWER, then discard
      if (delta < 0)
      {
         Warning(("Version in file is new.  Discarding tag\n"));
         vfclose (file);
         file = NULL;
         return; 
      }
   }


   // read in tag table
   vfseek(file,header.table,SEEK_SET);
   table.Read(file);
   
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
   vfseek(file,block->offset,SEEK_SET);

   BufTagFileBlockHeader header;
   int len = vfread(&header,1,sizeof(header),file);
   if (len != sizeof(header))
   { 
      Warning(("BufTagFileRead::OpenBlock(): read only %d of %d bytes\n",len,sizeof(header)));
      SetCurBlock(NULL);
      return E_FAIL;
   }
   
   // compare versons
   int delta = VersionNumsCompare(version,&header.version);
   if (delta != 0)
   {
#ifdef DBG_ON
      char buf[256];
      strcpy(buf,VersionNum2String(version));
      ConfigSpew("tagfile_spew",("Tag data %s is not current version.\nOld: %s, New: %s\n",
               tag->label,VersionNum2String(&header.version),buf));
#endif 

      // if it's NEWER, then discard
      if (delta < 0)
      {
         ConfigSpew("tagfile_spew",("Version in file is newer.  Discarding tag\n"));
         SetCurBlock(NULL);
         return E_FAIL;
      }
   }
   
   *version = header.version;
   blockptr = 0;

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

   ulong bytesleft = block->size - blockptr; 
   if (buflen > bytesleft)
      buflen = max(bytesleft,0);
   
   int len = vfread(buf,1,buflen,file);
   blockptr += len;
   return len;
}

////////////////////////////////////////

STDMETHODIMP_(long) BufTagFileRead::Write(const char* , int )
{
   Warning(("Writing to a tagfile opened for reading\n"));
   return -1;
}

////////////////////////////////////////

STDMETHODIMP BufTagFileRead::Move(char* buf, int buflen)
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
