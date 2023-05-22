/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/prj/cam/src/RCS/memtfil_.h 1.1 1998/01/21 00:21:57 mahk Exp $
#pragma once  
#ifndef __MEMTFIL__H
#define __MEMTFIL__H

#include <tagfile.h>
#include <memtfile.h>

#include <hashset.h>

//
// Memory Tag File Table
//

struct sMemTagEntry
{
   TagFileTag tag; 
   TagVersion version; 
   ulong block_size;
   ulong buf_size;
   uchar* buf; 
};

class cMemTagTable : public cStrHashSet<sMemTagEntry* >
{
protected:
   virtual tHashSetKey GetKey(tHashSetNode node) const
   {
      sMemTagEntry* e = (sMemTagEntry*)node;
      return (tHashSetKey)&e->tag.label; 
   }

public:
   enum 
   {
      kDefaultBufSize = 1024,
   };

   virtual ~cMemTagTable();

   sMemTagEntry* AddTag(const TagFileTag* tag, const TagVersion* version);

}; 

////////////////////////////////////////////////////////////
// Tag File Base 
//


class cMemFile : public cCTUnaggregated<ITagFile,&IID_ITagFile,kCTU_Default>
{
protected:
   cMemTagTable* mpTable; 
   sMemTagEntry* mpCurBlock;
   ulong mCursor; 

   static constexpr TagVersion Version = { 0, 1 };

public:
   cMemFile(cMemTagTable* table) 
      :mpCurBlock(NULL),
       mCursor(0),
       mpTable(table)
   {}; 

   virtual ~cMemFile() {}; 

   STDMETHOD_(const TagVersion*, GetVersion)() { return &Version;};  
   STDMETHOD_(ulong, BlockSize)(const TagFileTag*); 
   STDMETHOD_(const TagFileTag*, CurrentBlock)();
   STDMETHOD(Seek)(ulong offset, TagFileSeekMode from);
   STDMETHOD_(ulong,Tell)(); 
   STDMETHOD_(ulong,TellFromEnd)(); 
   STDMETHOD_(ITagFileIter*,Iterate)();
   
};

////////////////////////////////////////////////////////////
// READ/WRITE TAG FILE CLASS
//

class cMemFileReadWrite : public cMemFile
{
   TagFileOpenMode mMode;
public:
   cMemFileReadWrite(cMemTagTable* table, TagFileOpenMode mode)
      : cMemFile(table),mMode(mode) {}; 

   ~cMemFileReadWrite() {}; 

   STDMETHOD_(TagFileOpenMode,OpenMode)() { return mMode; }; 

   // BLOCK OPS
   STDMETHOD(OpenBlock)(const TagFileTag* tag, TagVersion* version);
   STDMETHOD(CloseBlock)();

   // Read/write/seek operations
   STDMETHOD_(long,Read)(char* buf, int buflen);
   STDMETHOD_(long,Write)(const char* buf, int buflen);
   STDMETHOD_(long,Move)(char* buf, int buflen);
};




#endif // __MEMTFIL__H
