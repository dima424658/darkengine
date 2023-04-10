/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ui/movie.h,v 1.5 2000/01/31 10:04:59 adurant Exp $
//
// Movie playing APIs
//
#pragma once

#ifndef __MOVIE_H
#define __MOVIE_H

#include <movieapi.h>

EXTERN BOOL MoviePlaySynchronous(const char * pszFile, int volume);
EXTERN void MovieOnExit();

// Pass in -1 for w and h to auto set from the movie
EXTERN BOOL MoviePlayAsynchronous(const char * pszFile,int x,int y,int w,int h, int volume);

// Get the current status
EXTERN eMP1State MovieStatus();

#endif /* !__MOVIE_H */
