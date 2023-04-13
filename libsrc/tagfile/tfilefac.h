/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/tagfile/RCS/tfilefac.h 1.1 1998/09/28 13:50:45 CMONTER Exp $
#pragma once  
#ifndef __TFILEFAC_H
#define __TFILEFAC_H
//
// This is the API for specifying an alternate source of tag files to be used
// and dbLoad
//
typedef ulong edbFiletype;

//
// Tag-file-based dbFile API 
//
EXTERN edbFiletype dbSaveTagFile(ITagFile* file, edbFiletype filetype);
EXTERN edbFiletype dbLoadTagFile(ITagFile* file, edbFiletype filetype);
EXTERN edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype);

class cTagFileFactory
{
public:
   virtual ~cTagFileFactory() {};

   // Open a file
   virtual ITagFile* Open(const char* filename, TagFileOpenMode mode) = 0;
};

#endif // __TFILEFAC_H
