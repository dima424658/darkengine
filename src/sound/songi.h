/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songi.h,v 1.5 2000/01/31 10:02:41 adurant Exp $
#pragma once

#ifndef SONGI_H
#define SONGI_H

#include <comtools.h>
#include <song.h>
#include <dynarray.h>
#include <songid.h>

class cSong : public ISong
{
   DECLARE_UNAGGREGATABLE();

public:
   cSong();
   virtual ~cSong();

   STDMETHOD_(BOOL, SetSongInfo)(THIS_ sSongInfo* pSongInfo);
   STDMETHOD_(BOOL, GetSongInfo)(THIS_ sSongInfo* ppSongInfo);

   STDMETHOD_(BOOL, AddSection)(THIS_ ISongSection* pSection);
   STDMETHOD_(unsigned, CountSections) (THIS);
   STDMETHOD_(BOOL, SetSection)(THIS_ unsigned index, ISongSection* pSection);
   STDMETHOD_(BOOL, GetSection)(THIS_ unsigned index, ISongSection** ppSection);

   STDMETHOD_(BOOL, AddEvent)(THIS_ ISongEvent* pEvent);
   STDMETHOD_(unsigned, CountEvents)(THIS);
   STDMETHOD_(BOOL, GetEvent)(THIS_ unsigned index, ISongEvent** ppEvent);

private:
   sSongInfo                        m_songInfo;
   cDynArray <ISongSection*>        m_sections;
   cDynArray <ISongEvent*>          m_events;
};

class cSongSection : public ISongSection
{
   DECLARE_UNAGGREGATABLE();

public:
   cSongSection();
   virtual ~cSongSection();

   STDMETHOD_(BOOL, SetSectionInfo)(THIS_ sSongSectionInfo* pSectionInfo);
   STDMETHOD_(BOOL, GetSectionInfo)(THIS_ sSongSectionInfo* pSectionInfo);
   
   STDMETHOD_(BOOL, AddSample)(THIS_ ISongSample* pSample);
   STDMETHOD_(unsigned, CountSamples)(THIS);
   STDMETHOD_(BOOL, GetSample)(THIS_ unsigned index, ISongSample** ppSample);

   STDMETHOD_(BOOL, AddEvent)(THIS_ ISongEvent* pEvent);
   STDMETHOD_(unsigned, CountEvents)(THIS);
   STDMETHOD_(BOOL, GetEvent)(THIS_ unsigned index, ISongEvent** ppEvent);

private:
   sSongSectionInfo                 m_sectionInfo;
   cDynArray <ISongSample*>         m_samples;
   cDynArray <ISongEvent*>          m_events;
};

class cSongSample : public ISongSample
{
   DECLARE_UNAGGREGATABLE();

public:
   cSongSample();
   virtual ~cSongSample();

   STDMETHOD_(BOOL, SetSampleInfo)(THIS_ sSongSampleInfo* pSampleInfo);
   STDMETHOD_(BOOL, GetSampleInfo)(THIS_ sSongSampleInfo* pSampleInfo);

private:
   sSongSampleInfo                  m_sampleInfo;
};

class cSongEvent : public ISongEvent
{
   DECLARE_UNAGGREGATABLE();

public:
   cSongEvent();
   virtual ~cSongEvent();

   STDMETHOD_(BOOL, SetEventInfo)(THIS_ sSongEventInfo* pEventInfo);
   STDMETHOD_(BOOL, GetEventInfo)(THIS_ sSongEventInfo* pEventInfo);

   STDMETHOD_(BOOL, AddGoto)(THIS_ ISongGoto* pGoto);
   STDMETHOD_(unsigned, CountGotos)(THIS);
   STDMETHOD_(BOOL, GetGoto)(THIS_ unsigned index, ISongGoto** ppGoto);

private:
   sSongEventInfo                   m_eventInfo;
   cDynArray <ISongGoto*>           m_gotos;
};

class cSongGoto : public ISongGoto
{
   DECLARE_UNAGGREGATABLE();

public:
   cSongGoto();
   virtual ~cSongGoto();

   STDMETHOD_(BOOL, SetGotoInfo)(THIS_ sSongGotoInfo* pGotoInfo);
   STDMETHOD_(BOOL, GetGotoInfo)(THIS_ sSongGotoInfo* pGotoInfo);

private:
   sSongGotoInfo                    m_gotoInfo;
};

#endif

