/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songplr.h,v 1.11 2000/02/20 15:12:40 patmac Exp $
#pragma once

#ifndef SONGPLR_H
#define SONGPLR_H

#include <comtools.h>
#include <song.h>
#include <lgsound.h>
#include <storeapi.h>

F_DECLARE_INTERFACE(ISongPlayer);

#ifdef __cplusplus
extern "C" {
#endif

BOOL CreateSongPlayer (ISongPlayer** ppPlayer, ISndMixer* pSndMixer, ISearchPath* pWavPath, IUnknown *pOuter, int songSndGroup);

// ISongPlayer
#undef INTERFACE
#define INTERFACE ISongPlayer

DECLARE_INTERFACE_(ISongPlayer, IUnknown)
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, SetSong)(THIS_ ISong* pSong) PURE;
   STDMETHOD_(void, GetSong)(THIS_ ISong** ppSong) PURE;
   STDMETHOD_(void, UnloadSong)(THIS) PURE;
   STDMETHOD_(BOOL, StartPlaying)(THIS) PURE;
   STDMETHOD_(BOOL, StopPlaying)(THIS) PURE;
   STDMETHOD_(BOOL, PausePlaying)(THIS) PURE;
   STDMETHOD_(BOOL, ResumePlaying)(THIS) PURE;
   STDMETHOD_(void, SetVolume)(THIS_ int vol) PURE;
   STDMETHOD_(int, GetVolume)(THIS) PURE;
   STDMETHOD_(BOOL, SendEvent)(THIS_ const char* pEventString, int proiority) PURE;
   STDMETHOD_(void, SetThemed)(THIS_ BOOL isThemed) PURE;
   STDMETHOD_(BOOL, IsThemed)(THIS) PURE;
   STDMETHOD_(BOOL, SetTheme)(THIS_ const char* pThemeString) PURE;
   STDMETHOD_(void, GetMemento)(THIS_ void **pRetData, int *pRetSize ) PURE;
   STDMETHOD_(BOOL, RestoreMemento)(THIS_ void *pData, int size ) PURE;
};

   /*
#define ISongPlayer_StartPlaying(p, a)                COMCall1(p, StartPlaying, a)
#define ISongPlayer_SendEvent(p, a)                   COMCall1(p, SendEvent, a)
   */

#ifdef __cplusplus
}
#endif

#endif

