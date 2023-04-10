/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/findhack.h,v 1.2 2000/01/29 12:41:15 adurant Exp $
// misc silliness for abstracting the find stuff
#pragma once

#ifndef __FINDHACK_H
#define __FINDHACK_H

// do we want a NAME_MAX or PATH_MAX for LG_

// I guess this should probably be in a header somewhere
#ifndef __WATCOMC__
# if defined(__OS2__) || defined(__NT__)
#   define NAME_MAX  255      /* maximum filename for HPFS or NTFS */
#   define PATH_MAX  259 /* maximum length of full pathname excl. '\0' */
# else
#   define NAME_MAX  12    /* 8 chars + '.' +  3 chars */
#   define PATH_MAX  143 /* maximum length of full pathname excl. '\0' */
# endif
#endif

// well, limits.h has a NAME and PATH MAX, but only if POSIX defined
// so, who knows

#ifndef _MSC_VER
#include <dos.h>
#include <direct.h>
#define lg_find_declare(fs)              struct find_t fs
#define lg_findfirst_p(file,fs_ptr)      (_dos_findfirst(file,0,fs_ptr)==0)
#define lg_findnext_p(fs_ptr)            (_dos_findnext(fs_ptr)==0)
#define lg_findclose(fs_ptr)             _dos_findclose(fs_ptr)
#define lg_find_attrib(fs)               (fs.attrib)
#define lg_find_name(fs)                 (fs.name)
#define lg_find_size(fs)                 (fs.size)
#else
#include <io.h>
#include <limits.h>
#define lg_find_declare(fs)              struct _finddata_t fs; long lg_find_hnd
#define lg_findfirst_p(file,fs_ptr)      ((lg_find_hnd=_findfirst(file,fs_ptr))!=-1)
#define lg_findnext_p(fs_ptr)            (_findnext(lg_find_hnd,fs_ptr)==0)
#define lg_findclose(fs_ptr)             _findclose(lg_find_hnd)
#define lg_find_attrib(fs)               (fs.attrib)
#define lg_find_name(fs)                 (fs.name)
#define lg_find_size(fs)                 (fs.size)
#endif

#endif  // __FINDHACK_H
