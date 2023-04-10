/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songutil.h,v 1.10 2000/02/20 15:12:42 patmac Exp $
#pragma once

//
// Songutil is basically a quick and dirty top level song system
// API.  It manages the control over a single song.
//

#ifndef SONGUTIL_H
#define SONGUTIL_H

#include <lgsound.h>
#include <tagfile.h>

#ifdef __cplusplus
extern "C" {
#endif

void SongUtilInit (ISndMixer* pSndMixer);
void SongUtilShutdown ();
void SongUtilDisableMusic ();
void SongUtilEnableMusic ();
void SongUtilSetTheme (const char* pThemeStr);
void SongUtilLoad (const char *pFilename);
void SongUtilPlay ();
void SongUtilStop ();
void SongUtilPause ();
void SongUtilResume ();
void SongUtilSetVolume (int vol);
int SongUtilGetVolume ();
void SongUtilEvent (const char* pEventString);

BOOL SongUtilSaveMission(ITagFile *file);
BOOL SongUtilLoadMission(ITagFile *file);
void SongUtilResetMission();

void SongUtilDumpMono();

void SongUtilSetSoundGroup( int songGroup );

#ifdef __cplusplus
}
#endif

#endif
