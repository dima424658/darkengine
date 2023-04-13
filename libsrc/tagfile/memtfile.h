/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/tagfile/RCS/memtfile.h 1.1 1998/09/28 13:50:38 CMONTER Exp $
#pragma once  
#ifndef __MEMTFILE_H
#define __MEMTFILE_H

#include <tFileFac.h>

//
// A "memory tag file" factory. 
//

// All tag files created with this factory are the same "file" 
// The "file" exists in memory.  Create a "write mode" file to write out 
// some data to a memory buffer, and then create a "read mode" file to read it
// back in. 


extern cTagFileFactory* CreateMemoryTagFileFactory(void);


#endif // __MEMTFILE_H


