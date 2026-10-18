/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/prj/cam/src/RCS/tagfile_.h 1.7 1997/11/19 21:42:21 mahk Exp $

#include <hashset.h>
#include <cstdio>
#include <stdint.h>

#include <tagfile.h>

////////////////////////////////////////////////////////////
// TAG FILE IMPLEMENTATION
//

typedef ulong Offset;
#define BAD_OFFSET 0xFFFFFFFF

//============================================================
// TAG FILE HEADERS
//

struct TagFileHeader
{
   Offset table;           // tag table offset
   TagVersion version;        // file format version
   char pad[256];          // space for expansion
   ubyte deadbeef[4];      // 0xDEADBEEF if not corrupt

   TagFileHeader() { memset(this,0,sizeof(*this)); } ; 
};

//
// Block header
//

struct TagFileBlockHeader
{
   TagFileTag tag;
   TagVersion version;
   ulong size; // block size

   TagFileBlockHeader() { memset(this,0,sizeof(*this));};
};

static_assert(sizeof(TagFileHeader) == 272,
   "TagFileHeader must retain its 32-bit on-disk layout");
static_assert(sizeof(TagFileBlockHeader) == 24,
   "TagFileBlockHeader must retain its 32-bit on-disk layout");


//============================================================
// TAG TABLE CLASS
//

//
// Hash functions
//
struct TagHashFunctions
{
   static unsigned Hash(const TagFileTag* p) { return HashString(p->label); }; 
   static BOOL IsEqual(const TagFileTag* p, const TagFileTag* q) 
      { return !strcmp(p->label,q->label); } ; 
};

struct TagFileBlockHeader;
//
// Entry
//
struct TagTableEntry
{
   TagFileTag key;
   Offset   offset; 
   slong   size;

   TagTableEntry(const TagFileTag& k, Offset off,slong sz = 0) :key(k),offset(off),size(sz) {} ;
   TagTableEntry() :offset(BAD_OFFSET),size(-1) { key.label[0] = '\0';}; 

   uint64_t Start() const { return (uint64_t)offset + sizeof(struct TagFileBlockHeader); } ;
   uint64_t End() const { return Start() + (uint64_t)size; } ;
};

static_assert(sizeof(TagTableEntry) == 20,
   "TagTableEntry must retain its 32-bit on-disk layout");


//
// Table
//
class TagFileTable : public cHashSet<TagTableEntry *,const TagFileTag *,TagHashFunctions>
{
protected:
   virtual tHashSetKey GetKey(tHashSetNode) const;

public:
   virtual ~TagFileTable();

   void Write(FILE* file);  // write to file 
   bool Read(FILE* file, uint64_t fileSize, uint64_t tableOffset);
}; 


//============================================================
// BASE TAG FILE CLASS
// 
// Implements little, except for version info. 
//

class TagFileBase : public ITagFile
{
protected:
   TagFileTable table;                    // Tag table
   FILE* file;                            // file 
   TagTableEntry* block;                  // current block
   ulong blockptr;                        // file ptr within block
   TagVersion blockVersion;               // version of the open read block
   uint64_t fileSize;                      // validated physical file length

public:
   TagFileBase(FILE* f = NULL) :file(f),block(NULL),blockVersion{0,0},fileSize(0) {} ;
   TagFileBase(const char* fname, const char* mode)
      : file(fopen(fname,mode)),block(NULL),blockVersion{0,0},fileSize(0) {} ;
   virtual ~TagFileBase();

   static ITagFile* Open(const char* filename, TagFileOpenMode mode);
   
   // IUnknown Methods
   DECLARE_UNAGGREGATABLE(); 

   // Basic operations   
   STDMETHOD_(const TagVersion*, GetVersion)(); 
   STDMETHOD_(ulong, BlockSize)(const TagFileTag*); 
   STDMETHOD_(const TagFileTag*, CurrentBlock)();
   STDMETHOD(Seek)(ulong offset, TagFileSeekMode from);
   STDMETHOD_(ulong,Tell)(); 
   STDMETHOD_(ulong,TellFromEnd)(); 
   STDMETHOD_(ITagFileIter*,Iterate)();
   
protected:
   // Set the current block
   void SetCurBlock(const TagFileTag* tag);

}; 



//============================================================
// WRITE TAG FILE CLASS
// 
// Implements writing, not reading.  Duh. 
//

class TagFileWrite : public TagFileBase
{

public:
   TagFileWrite(const char* fn);
   virtual ~TagFileWrite();
   
   STDMETHOD_(TagFileOpenMode,OpenMode)();

   // BLOCK OPS
   STDMETHOD(OpenBlock)(const TagFileTag* tag, TagVersion* version);
   STDMETHOD(CloseBlock)();

   // Read/write/seek operations
   STDMETHOD_(long,Read)(char* buf, int buflen);
   STDMETHOD_(long,Write)(const char* buf, int buflen);
   STDMETHOD_(long,Move)(char* buf, int buflen);
}; 


//============================================================
// READ TAG FILE CLASS
// 
// The yin to the previous class' yang.
//

class TagFileRead : public TagFileBase
{
public:
   TagFileRead(const char* fn);
   virtual ~TagFileRead();
   
   STDMETHOD_(TagFileOpenMode,OpenMode)();

   // BLOCK OPS
   STDMETHOD(OpenBlock)(const TagFileTag* tag, TagVersion* version);
   STDMETHOD(CloseBlock)();

   // Read/write/seek operations
   STDMETHOD_(long,Read)(char* buf, int buflen);
   STDMETHOD_(long,Write)(const char* buf, int buflen);
   STDMETHOD_(long,Move)(char* buf, int buflen);
}; 

