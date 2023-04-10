/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songi.cpp,v 1.5 2000/02/19 12:37:16 toml Exp $

#include <lg.h>
#include <song.h>
#include <songi.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Factory methods for creating SongParser objects.
BOOL CreateSong (ISong **ppSong, IUnknown *pOuter)
{
   *ppSong = NULL;
   if( pOuter != NULL )
      return FALSE;

   *ppSong = new cSong;
   return (*ppSong != NULL);
}

BOOL CreateSongSection (ISongSection **ppSection, IUnknown *pOuter)
{
   *ppSection = NULL;
   if( pOuter != NULL )
      return FALSE;

   *ppSection = new cSongSection;
   return (*ppSection != NULL);
}

BOOL CreateSongSample (ISongSample **ppSample, IUnknown *pOuter)
{
   *ppSample = NULL;
   if( pOuter != NULL )
      return FALSE;

   *ppSample = new cSongSample;
   return (*ppSample != NULL);
}

BOOL CreateSongEvent (ISongEvent **ppEvent, IUnknown *pOuter)
{
   *ppEvent = NULL;
   if( pOuter != NULL )
      return FALSE;

   *ppEvent = new cSongEvent;
   return (*ppEvent != NULL);
}

BOOL CreateSongGoto (ISongGoto **ppGoto, IUnknown *pOuter)
{
   *ppGoto = NULL;
   if( pOuter != NULL )
      return FALSE;

   *ppGoto = new cSongGoto;
   return (*ppGoto != NULL);
}


//
// cSong
//
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSong, ISong);

cSong::cSong ()
{
   m_songInfo.id[0] = '\0';
}

cSong::~cSong ()
{
   // Have to call Release() on each thing I contain.
   unsigned count, i;
   count = m_sections.Size();
   for (i = 0; i < count; i++)
      m_sections[i]->Release();

   count = m_events.Size();
   for (i = 0; i < count; i++)
      m_events[i]->Release();
}

STDMETHODIMP_(BOOL)
cSong::SetSongInfo (sSongInfo* pSongInfo)
{
   m_songInfo = *pSongInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSong::GetSongInfo (sSongInfo* pSongInfo)
{
   *pSongInfo = m_songInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSong::AddSection (ISongSection* pSection)
{
   pSection->AddRef();
   m_sections.Append(pSection);
   return TRUE;
}

STDMETHODIMP_(unsigned)
cSong::CountSections ()
{
   return m_sections.Size();
}

STDMETHODIMP_(BOOL)
cSong::SetSection (unsigned index, ISongSection* pSection)
{
   Assert_ (index < m_sections.Size());

   // Release any existing section.
   SafeRelease (m_sections[index]);

   // Assign new.
   pSection->AddRef();
   m_sections[index] = pSection;

   return TRUE;
}

STDMETHODIMP_(BOOL)
cSong::GetSection (unsigned index, ISongSection** ppSection)
{
   Assert_ (index < m_sections.Size());

   ISongSection* pSection = m_sections[index];
   pSection->AddRef();
   *ppSection = pSection;

   return TRUE;
}

STDMETHODIMP_(BOOL)
cSong::AddEvent (ISongEvent* pEvent)
{
   pEvent->AddRef();
   m_events.Append(pEvent);
   return TRUE;
}

STDMETHODIMP_(unsigned)
cSong::CountEvents ()
{
   return m_events.Size();
}

STDMETHODIMP_(BOOL)
cSong::GetEvent (unsigned index, ISongEvent** ppEvent)
{
   Assert_ (index < m_events.Size());

   ISongEvent* pEvent = m_events[index];
   pEvent->AddRef();
   *ppEvent = pEvent;

   return TRUE;
}

//
// cSongSection
//
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSongSection, ISongSection);

cSongSection::cSongSection ()
{
   m_sectionInfo.id[0] = '\0';
   m_sectionInfo.volume = 0;
   m_sectionInfo.loopCount = 1;
}

cSongSection::~cSongSection ()
{
   // Have to call Release() on each thing I contain.
   unsigned count, i;

   count = m_samples.Size();
   for (i = 0; i < count; i++)
      m_samples[i]->Release();

   count = m_events.Size();
   for (i = 0; i < count; i++)
      m_events[i]->Release();
}

STDMETHODIMP_(BOOL)
cSongSection::SetSectionInfo (sSongSectionInfo* pSectionInfo)
{
   m_sectionInfo = *pSectionInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongSection::GetSectionInfo (sSongSectionInfo* pSectionInfo)
{
   *pSectionInfo = m_sectionInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongSection::AddSample (ISongSample* pSample)
{
   pSample->AddRef();
   m_samples.Append(pSample);
   return TRUE;
}

STDMETHODIMP_(unsigned)
cSongSection::CountSamples ()
{
   return m_samples.Size();
}

STDMETHODIMP_(BOOL)
cSongSection::GetSample (unsigned index, ISongSample** ppSample)
{
   Assert_ (index < m_samples.Size());

   ISongSample* pSample = m_samples[index];
   pSample->AddRef();
   *ppSample = pSample;

   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongSection::AddEvent (ISongEvent* pEvent)
{
   pEvent->AddRef();
   m_events.Append(pEvent);
   return TRUE;
}

STDMETHODIMP_(unsigned)
cSongSection::CountEvents ()
{
   return m_events.Size();
}

STDMETHODIMP_(BOOL)
cSongSection::GetEvent (unsigned index, ISongEvent** ppEvent)
{
   Assert_ (index < m_events.Size());

   ISongEvent* pEvent = m_events[index];
   pEvent->AddRef();
   *ppEvent = pEvent;

   return TRUE;
}


//
// cSongSample
//
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSongSample, ISongSample);

cSongSample::cSongSample ()
{
   m_sampleInfo.name[0] = '\0';
   //m_sampleInfo.loopCount = 1;
   //m_sampleInfo.probability = 100;
}

cSongSample::~cSongSample ()
{
}

STDMETHODIMP_(BOOL)
cSongSample::SetSampleInfo (sSongSampleInfo* pSampleInfo)
{
   m_sampleInfo = *pSampleInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongSample::GetSampleInfo (sSongSampleInfo* pSampleInfo)
{
   *pSampleInfo = m_sampleInfo;
   return TRUE;
}


//
// cSongEvent
//
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSongEvent, ISongEvent);

cSongEvent::cSongEvent ()
{
   m_eventInfo.eventString[0] = '\0';
   m_eventInfo.flags = 0;
}

cSongEvent::~cSongEvent ()
{
   // Have to call Release() on each thing I contain.
   unsigned count, i;

   count = m_gotos.Size();
   for (i = 0; i < count; i++)
      m_gotos[i]->Release();
}

STDMETHODIMP_(BOOL)
cSongEvent::SetEventInfo (sSongEventInfo* pEventInfo)
{
   m_eventInfo = *pEventInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongEvent::GetEventInfo (sSongEventInfo* pEventInfo)
{
   *pEventInfo = m_eventInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongEvent::AddGoto (ISongGoto* pGoto)
{
   pGoto->AddRef();
   m_gotos.Append(pGoto);
   return TRUE;
}

STDMETHODIMP_(unsigned)
cSongEvent::CountGotos ()
{
   return m_gotos.Size();
}

STDMETHODIMP_(BOOL)
cSongEvent::GetGoto (unsigned index, ISongGoto** ppGoto)
{
   Assert_ (index < m_gotos.Size());

   ISongGoto* pGoto = m_gotos[index];
   pGoto->AddRef();
   *ppGoto = pGoto;

   return TRUE;
}


//
// cSongGoto
//
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSongGoto, ISongGoto);

cSongGoto::cSongGoto()
{
   m_gotoInfo.sectionIndex = -1;
   m_gotoInfo.probability = 100;
}

cSongGoto::~cSongGoto()
{
}

STDMETHODIMP_(BOOL)
cSongGoto::SetGotoInfo (sSongGotoInfo* pGotoInfo)
{
   m_gotoInfo = *pGotoInfo;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongGoto::GetGotoInfo (sSongGotoInfo* pGotoInfo)
{
   *pGotoInfo = m_gotoInfo;
   return TRUE;
}

