/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbtagfil.h,v 1.3 1999/01/11 11:38:54 mahk Exp $
#pragma once  
#ifndef __DBTAGFIL_H
#define __DBTAGFIL_H

#include <tagfile.h>
#include <dbtype.h>

//
// Tag-file-based dbFile API 
//
EXTERN edbFiletype dbSaveTagFile(ITagFile* file, edbFiletype filetype);
EXTERN edbFiletype dbLoadTagFile(ITagFile* file, edbFiletype filetype);
EXTERN edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype);

//
// This is the API for specifying an alternate source of tag files to be used by dbSave 
// and dbLoad
//

class cDBFileFactory
{
public:
   virtual ~cDBFileFactory() {}; 

   // Open a file
   virtual ITagFile* Open(const char* filename, TagFileOpenMode mode) = 0; 
}; 

//
// Set the current tag file factory
// returns the old one
//

extern cDBFileFactory* dbSetFileFactory(cDBFileFactory* new_factory); 

//
// Open a file using the current tag file factory
//

EXTERN ITagFile* dbOpenFile(const char* filename, TagFileOpenMode mode); 

#endif // __DBTAGFIL_H





