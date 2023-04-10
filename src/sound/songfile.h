/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songfile.h,v 1.2 2000/01/31 10:02:40 adurant Exp $
#pragma once

// (MRW : Stolen from schfile.h)

#ifndef __SONGFILE_H
#define __SONGFILE_H

#include <song.h>

#ifdef EDITOR

// function for reading a schema file
typedef void (*fSongRead)(char *filename);

// read all files from res path and local dir that match "what" string
void SongFilesLoadFromDir(const char *where, const char *what, fSongRead readFunc);

void SongSave (ISong* pSong, char* filename);
#endif // EDITOR

ISong* SongLoad (char* filename);

#endif // Songfile_h
