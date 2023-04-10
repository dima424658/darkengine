/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songplri.h,v 1.13 2000/02/20 15:12:39 patmac Exp $
#pragma once

#ifndef SONGPLRI_H
#define SONGPLRI_H

#include <comtools.h>
#include <songplr.h>
#include <song.h>
#include <lgsound.h>
#include <sndsrc.h>
#include <storeapi.h>
#include <resapi.h>

// stuff saved by GetMemento
typedef struct {
   int      currSegGate;
   int      nextSegGate;
   int      loopCountGate;
   uint32   lastBranch;
   char     lastEvent[32];
   int      lastPriority;
   char     lastTheme[32];
} sSongPlayerSaveState;

class cSongPlayer : public ISongPlayer
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNAGGREGATABLE();

public:
   cSongPlayer (ISndMixer* pSndMixer, ISearchPath* pWavPath, int soundGroup);
   virtual ~cSongPlayer();
   
   STDMETHOD_(BOOL, SetSong)(THIS_ ISong* pSong);
   STDMETHOD_(void, GetSong)(THIS_ ISong** ppSong);
   STDMETHOD_(void, UnloadSong)(THIS);
   STDMETHOD_(BOOL, StartPlaying)(THIS);
   STDMETHOD_(BOOL, StopPlaying)(THIS);
   STDMETHOD_(BOOL, PausePlaying)(THIS);
   STDMETHOD_(BOOL, ResumePlaying)(THIS);
   STDMETHOD_(void, SetVolume)(THIS_ int vol);
   STDMETHOD_(int, GetVolume)(THIS);
   STDMETHOD_(BOOL, SendEvent)(THIS_ const char* pEventString, int proiority);
   STDMETHOD_(void, SetThemed)(THIS_ BOOL isThemed);
   STDMETHOD_(BOOL, IsThemed)(THIS);
   STDMETHOD_(BOOL, SetTheme)(THIS_ const char* pThemeString);
   STDMETHOD_(void, GetMemento)(THIS_ void **pRetData, int *pRetSize );
   STDMETHOD_(BOOL, RestoreMemento)(THIS_ void *pData, int size );

   void _HandleEventMatch(ISongEvent* pEvent);
   void _DoSegmentCallback();
   void _DoSongEndCallback();

private:
   ISong*         m_pSong;
   ISearchPath*   m_pWavPath;
   ISndMixer*     m_pSndMixer;
   ISndSample*    m_pSndSample;
   ISndSource*    m_pSndSrc;
   IRes**         m_pResArray;
   char*          m_pStreamBuffer;
   int            m_currPriority;
   BOOL           m_isPlaying;
   BOOL           m_isThemed;
   int            m_volume;
   char           m_currTheme[32];
   BOOL           m_isPaused;
   char           m_lastEvent[32];
   int            m_lastPriority;
   sSongPlayerSaveState m_memento;
   BOOL           m_bHaveMemento;
   int            m_soundGroup;
};

#endif
