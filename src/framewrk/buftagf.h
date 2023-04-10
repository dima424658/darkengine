/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/buftagf.h,v 2.1 2000/02/26 15:40:06 toml Exp $

//
// A copy of tech\libsrc\tagfile! what a hack! (toml 02-23-00)
//

#include <comtools.h>
#include <hashset.h>
#include <tagfile.h>

#ifndef __BUFTAGF_H
#define __BUFTAGF_H

struct VFILE;

////////////////////////////////////////////////////////////
// TAG FILE IMPLEMENTATION
//

typedef ulong Offset;
#define BAD_OFFSET 0xFFFFFFFF

//============================================================
// TAG FILE HEADERS
//

struct BufTagFileHeader
{
   Offset table;           // tag table offset
   TagVersion version;        // file format version
   char pad[256];          // space for expansion
   ubyte deadbeef[4];      // 0xDEADBEEF if not corrupt

   BufTagFileHeader() { memset(this,0,sizeof(*this)); } ; 
};

//
// Block header
//

struct BufTagFileBlockHeader
{
   TagFileTag tag;
   TagVersion version;
   ulong size; // block size

   BufTagFileBlockHeader() { memset(this,0,sizeof(*this));};
};


//============================================================
// TAG TABLE CLASS
//

//
// Hash functions
//
struct BufTagHashFunctions
{
   static unsigned Hash(const TagFileTag* p) { return HashString(p->label); }; 
   static BOOL IsEqual(const TagFileTag* p, const TagFileTag* q) 
      { return !strcmp(p->label,q->label); } ; 
};

struct BufTagFileBlockHeader;
//
// Entry
//
struct BufTagTableEntry
{
   TagFileTag key;
   Offset   offset; 
   long    size;

   BufTagTableEntry(const TagFileTag& k, Offset off,long sz = 0) :key(k),offset(off),size(sz) {} ;
   BufTagTableEntry() :offset(BAD_OFFSET),size(-1) { key.label[0] = '\0';}; 

   Offset Start() { return offset + sizeof(struct BufTagFileBlockHeader); } ; 
   Offset End() { return Start() + size; } ; 
};


//
// Table
//
class BufTagFileTable : public cHashSet<BufTagTableEntry *,const TagFileTag *,BufTagHashFunctions>
{
protected:
   virtual tHashSetKey GetKey(tHashSetNode) const;

public:
   virtual ~BufTagFileTable();

   void Write(VFILE* file);  // write to file 
   void Read(VFILE* file);   // read from file
}; 


//============================================================
// BASE TAG FILE CLASS
// 
// Implements little, except for version info. 
//

class BufTagFileBase : public ITagFile
{
protected:
   BufTagFileTable table;                    // Tag table
   BufTagTableEntry* block;                  // current block
   VFILE * file;
   ulong blockptr;                        // file ptr within block

public:
   BufTagFileBase() :file(NULL),block(NULL) {} ;
   BufTagFileBase(const char* fname, const char* mode);
   virtual ~BufTagFileBase();

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

   // convert offset to absolute
   // also check to see if a seek will step out of a box
   //  < 0 if before start, > 0 if after end, == 0 if inside, magnitude = how far out
   int PrepSeek(ulong& offset, TagFileSeekMode mode);  
}; 



//============================================================
// WRITE TAG FILE CLASS
// 
// Implements writing, not reading.  Duh. 
//

class BufTagFileWrite : public BufTagFileBase
{

public:
   BufTagFileWrite(const char* fn);
   virtual ~BufTagFileWrite();
   
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

class BufTagFileRead : public BufTagFileBase
{
public:
   BufTagFileRead(const char* fn);
   virtual ~BufTagFileRead();
   
   STDMETHOD_(TagFileOpenMode,OpenMode)();

   // BLOCK OPS
   STDMETHOD(OpenBlock)(const TagFileTag* tag, TagVersion* version);
   STDMETHOD(CloseBlock)();

   // Read/write/seek operations
   STDMETHOD_(long,Read)(char* buf, int buflen);
   STDMETHOD_(long,Write)(const char* buf, int buflen);
   STDMETHOD_(long,Move)(char* buf, int buflen);
}; 

#endif
