/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/tagfile/RCS/tagfile.h 1.2 1999/01/07 16:20:53 mahk Exp $

#ifndef __TAGFILE_H
#define __TAGFILE_H

#include <lg.h>
#include <io.h>

#include <comtools.h>

#include <tagguid.h>

#include <vernum.h>

////////////////////////////////////////////////////////////
// TAG FILE ABSTRACTION
//
// A tag file is a random-access file where the valid seek points are 
// marked by tags.
//

F_DECLARE_INTERFACE(ITagFile);
F_DECLARE_INTERFACE(ITagFileIter); 

//------------------------------------------------------------
// Tag Structure
//

typedef struct _TagFileTag
{
   char label[12]; // string name for the tag.
} TagFileTag; 

typedef struct VersionNum TagVersion;

//------------------------------------------------------------
// Open modes 
//

typedef enum TagFileOpenMode
{
   kTagOpenRead, // open for reading
   kTagOpenWrite, // open for writing (create or truncate as appropriate)
} TagFileOpenMode;

//------------------------------------------------------------
// Seek Modes
//

typedef enum TagFileSeekMode
{
   kTagSeekFromStart = SEEK_SET,
   kTagSeekFromHere  = SEEK_CUR, 
   kTagSeekFromEnd   = SEEK_END,     
} TagFileSeekMode;

enum TagFileBlockSizeConstants
{
   kTagNoSuchBlock = 0xFFFFFFFF,  // block size of an non-existant block
}; 

//////////////////////////////////////////////////////////////
// OPEN A TAG FILE
//

EXTERN ITagFile* TagFileOpen(const char* filename, TagFileOpenMode mode);

#undef INTERFACE
#define INTERFACE ITagFile

////////////////////////////////////////////////////////////
// TAG FILE COM INTERFACE
//

DECLARE_INTERFACE_(ITagFile, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

#define ITagFile_QueryInterface(p, a, b) COMQueryInterface(p, a, b)
#define ITagFile_AddRef(p)               COMAddRef(p)
#define ITagFile_Release(p)              COMRelease(p)


   //------------------------------------------------------------
   // Get the Current TagFile Version
   //

   STDMETHOD_(const TagVersion*, GetVersion)(THIS) PURE; 

#define ITagFile_GetVersion(p)          COMCall0(p, GetVersion)


   //------------------------------------------------------------
   // Get the open mode
   //

   STDMETHOD_(TagFileOpenMode,OpenMode)(THIS) PURE;

#define ITagFile_OpenMode(p)        COMCall0(p, OpenMode)


   //============================================================
   // TAG FILE BLOCK OPS
   //
   // These functions are for manipulating tagged blocks, which 
   // present themselves as "virtual files," in that one can seek 
   // and read/write within a block as is seen fit.
   //

   //------------------------------------------------------------
   // Open a new block.  Fails if the previously-opened block was not closed.  
   // 
   // In write mode: creates a new block at the end of the file.  Fails if a block 
   //                with the specified tag exists.
   //
   // In read mode: finds the tagged block in the file and seeks to the beginning.
   //               Fails if no block with that tag exists.   
   //
   //               Compares the version given with file's block version, 
   //               and then reads the file's block
   //               version into the given version. 
   //
   STDMETHOD(OpenBlock)(THIS_ const TagFileTag* tag, TagVersion* version) PURE;

#define ITagFile_OpenBlock(p, a, b)       COMCall2(p, OpenBlock, a, b)


   //------------------------------------------------------------
   // Close the current block.  Fails if no block is opened.
   // 
   // Reads and writes are not valid until a new block is opened.
   //
   STDMETHOD(CloseBlock)(THIS) PURE; 
   
#define ITagFile_CloseBlock(p)            COMCall0(p, CloseBlock)

   //------------------------------------------------------------
   // Get the tag for the current block
   //
   STDMETHOD_(const TagFileTag*, CurrentBlock)(THIS) PURE;

#define ITagFile_CurrentBlock(p)          COMCall0(p, CurrentBlock)

   //------------------------------------------------------------
   // Get the size of a block
   //
   STDMETHOD_(ulong, BlockSize)(THIS_ const TagFileTag*) PURE;

#define ITagFile_BlockSize(p, a)          COMCall1(p, BlockSize, a)

   //------------------------------------------------------------
   // Iterate over blocks 
   //
   STDMETHOD_(ITagFileIter*,Iterate)(THIS) PURE;

#define ITagFile_Iterate(p)               COMCall0(p, Iterate)

   //============================================================
   // READING, WRITING AND SEEKING
   //

   //------------------------------------------------------------
   // Seek to a point in the current block.  
   // Fails if the point sought is outside the current block. 
   //
   STDMETHOD(Seek)(THIS_ ulong, TagFileSeekMode) PURE; 

#define ITagFile_Seek(p, a, b)            COMCall2(p, Seek, a, b)
   

   //------------------------------------------------------------
   // Find the current offset into the block, from the front.
   // Results are undefined if there is no current block.
   //
   STDMETHOD_(ulong,Tell)(THIS) PURE;

#define ITagFile_Tell(p)                  COMCall0(p, Tell)

   //--------------------------------------------------------------
   // Return the number of bytes between the current file position
   // and the end of the block.  Note that this is oppositely signed
   // from the offest used in kTagSeekFromEnd.  Results are meaningless
   // if there is no current block. 
   //
   STDMETHOD_(ulong,TellFromEnd)(THIS) PURE;

   //------------------------------------------------------------
   // Read some data from a tag file.  Not valid in write mode.
   // Returns the actual number of bytes read, or < 0 for error
   //
   STDMETHOD_(long,Read)(THIS_ char* buf, int buflen) PURE;

#define ITagFile_Read(p, a, b)      COMCall2(p, Read, a, b)


   //------------------------------------------------------------
   // Write some data to a tag file.  Not valid in read mode.
   // Returns bytes written, or < 0 for error
   //
   STDMETHOD_(long,Write)(THIS_ const char* buf, int buflen) PURE;

#define ITagFile_Write(p, a, b)        COMCall2(p, Write, a, b)

   //------------------------------------------------------------
   // Either read or write, whichever is valid in the tag file's open mode
   //
   //
   STDMETHOD_(long,Move)(THIS_ char* buf, int buflen) PURE;

#define ITagFile_Move(p, a, b)      COMCall2(p, Move, a, b)

};

#undef INTERFACE 
#define INTERFACE ITagFileIter

DECLARE_INTERFACE_(ITagFileIter, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;

   // Pointer lasts until Next or Release
   STDMETHOD_(const TagFileTag*,Tag)(THIS) PURE;
}; 

#undef INTERFACE

#endif // __TAGFILE_H








