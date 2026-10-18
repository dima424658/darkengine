/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/findhack.h,v 1.2 2000/01/29 12:41:15 adurant Exp $
// misc silliness for abstracting the find stuff
#pragma once

#ifndef __FINDHACK_H
#define __FINDHACK_H

#include <platform_services.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define lg_find_declare(fs)              sPlatformFileFind fs = { 0 }
#define lg_findfirst_p(file,fs_ptr)      PlatformFindFirst(file,fs_ptr)
#define lg_findnext_p(fs_ptr)            PlatformFindNext(fs_ptr)
#define lg_findclose(fs_ptr)             PlatformFindClose(fs_ptr)
#define lg_find_attrib(fs)               (fs.attributes)
#define lg_find_name(fs)                 (fs.name)
#define lg_find_size(fs)                 (fs.size)

#endif  // __FINDHACK_H
