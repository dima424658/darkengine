/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/respaths.h,v 1.2 2000/01/31 09:48:39 adurant Exp $
//
// Globally-visible paths, which subsystems can make use of
#pragma once

#ifndef __RESPATHS_H
#define __RESPATHS_H

#include <storeapi.h>

//
// The global context path. This points to the major "roots": the top-level
// zipfile, or the CD-ROM top, or the current directory. Whatever. It is
// initialized in resapp.c, and subsystems should use it as their context
// path if they create their own paths.
//
EXTERN ISearchPath *gContextPath;

#endif // __RESPATHS_H
