/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songplri.cpp,v 1.17 2000/02/20 15:12:37 patmac Exp $

#include <songplr.h>
#include <songplri.h>
#include <sndsrc.h>
#include <playlist.h>
#include <appagg.h>
#include <storeapi.h>
#include <resapi.h>
#include <sndrstyp.h>
#include <auxsnd.h>
//#include <random.h> Nope... now using the random stuff in cam\src\framewrk
#include <rand.h>
#include <config.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


#ifndef SHIP
#define SONGPLR_TRACE(x) \
   if( config_is_defined( "songplr_trace" ) ) \
   { \
      mprintf( "In cSongPlayer::" ); \
      mprintf x; \
      mprintf( "\n" ); \
   }
#define SONGPLR_SPEW(x) if(config_is_defined("songplr_spew")) mprintf x
#else
#define SONGPLR_TRACE(x)
#define SONGPLR_SPEW(x)
#endif

//
// @HACK
// LAST MINUTE SHOCK UGLY HACK WARNING.
// The library sample.play() call doesn't return anything, so the usual
// way to deal is to set the sample's data to something and register a
// failure callback which sets a flag.  Unfortunately, the callback is
// per-mixer, not per-sample, so we have to use the damn appsfx-registered
// mixer callback, which expects the data field to be an sfx pointer.  So,
// we set up a structure so that it is physically aligned like an sfx so
// that it will set a field we can use.
//
struct fake_sfx_hack
{
   uint _dummy;
   uint _flags;
};
#define FAKE_SFXFLG_HAPPY       (1<<9)  // happy or not, ie. did i play
// End HACK

//
// Notes:
//
// We store m_pResArray because the playlist apparently does not know how to clean
// up after itself (not sure if this is true...).  If this is fixed, we will no
// longer have to store them here.
//

BOOL CreateSongPlayer (ISongPlayer **ppPlayer, ISndMixer* pSndMixer, ISearchPath* pWavPath, IUnknown *pOuter, int songSndGroup)
{
   *ppPlayer = NULL;
   if( pOuter != NULL )
      return FALSE;

   *ppPlayer = new cSongPlayer (pSndMixer, pWavPath, songSndGroup);
   return (*ppPlayer != NULL);
}

#define kSNGPLR_CurrSegGate     0
#define kSNGPLR_NextSegGate     1
#define kSNGPLR_LoopCountGate   2

// This is called at the beginning of each segment.
// Simply passes it on to a member function.
static void _segment_callback(ISndSource* /*pSndSource*/, uint32 *pCBD)
{
   cSongPlayer *pThis;
   long *pTmp;

   // skip first param (arg count), next param is our ptr
   pTmp = (long *) pCBD;
   pThis = (cSongPlayer*) pTmp[1];

   pThis->_DoSegmentCallback();
}

static void _song_end_callback(ISndSample* /*pSample*/, void *pCBD)
{
   cSongPlayer *pThis;

   pThis = (cSongPlayer*) pCBD;
   pThis->_DoSongEndCallback();
}

/*
// called when a sound stops
static void _sfx_end_callback(ISndSample *pSample, void *pCBD)
{
   _sfx *fx = (_sfx *) pCBD;

   if (fx)
      _sfx_cleanup_fx(fx, TRUE);

   pSample->Release();
   chanInUse--;
}
*/

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSongPlayer, ISongPlayer);

cSongPlayer::cSongPlayer(ISndMixer* pSndMixer, ISearchPath* pWavPath, int songSndGroup)
:  m_pSong(NULL),
   m_pSndSample(NULL),
   m_pSndSrc(NULL),
   m_pResArray(NULL),
   m_pStreamBuffer(NULL),
   m_currPriority(-1),
   m_isPlaying(FALSE),
   m_isThemed(FALSE),
   m_volume(0),
   m_isPaused(FALSE),
   m_lastPriority(-1),
   m_bHaveMemento(FALSE),
   m_soundGroup(songSndGroup)
{
   SONGPLR_TRACE(( "cSongPlayer()" ));

   m_currTheme[0] = '\0';
   pWavPath->AddRef();
   m_pWavPath = pWavPath;
   pSndMixer->AddRef();
   m_pSndMixer = pSndMixer;
   m_lastEvent[0] = '\0';
}

cSongPlayer::~cSongPlayer()
{
   SONGPLR_TRACE(( "~cSongPlayer()" ));

   if (NULL != m_pSndSample)
      m_pSndSample->Stop();

   UnloadSong ();

   SafeRelease (m_pSndMixer);
   SafeRelease (m_pWavPath);
}

STDMETHODIMP_(BOOL)
cSongPlayer::SetSong (ISong* pSong)
{
   SONGPLR_TRACE(( "SetSong(SONG)" ));

   unsigned i;
   ISongSection* pSection;
   ISongSample* pSample;
   sSongSampleInfo sampleInfo;

   // Stop song.  Will automatically unload song at song end callback.
   if (NULL != m_pSndSample)
      m_pSndSample->Stop();

   // Store the song in a member.
   pSong->AddRef();
   m_pSong = pSong;

   // Create the sound playlist/sample/source.
   uint numSegments = m_pSong->CountSections();
   m_pResArray = new IRes* [numSegments];

   AutoAppIPtr(ResMan);

   for (i = 0; i < numSegments; i++)
   {
      m_pSong->GetSection(i, &pSection);
      pSection->GetSample(0, &pSample); // @TBD : Multiple samples (layers).
      pSample->GetSampleInfo (&sampleInfo);
      pSample->Release();
      pSection->Release();

      m_pResArray[i] = pResMan->Bind(sampleInfo.name, RESTYPE_SOUND, m_pWavPath);
   }

   m_pSndSample = CreateSongSoundSource ( m_pSndMixer,
                                       numSegments,
                                       m_pResArray,
                                       &m_pStreamBuffer,
                                       _song_end_callback, // SndEndCallback      endCB,
                                       this, // void                *pEndCBData,
                                       &m_pSndSrc,
                                       _segment_callback,
                                       this);//&this);
   if ( m_pSndSample ) {
      m_pSndSample->SetGroup( m_soundGroup );
   }

   // we are not paused
   m_isPaused = FALSE;

   // Add ref to sound source.  COM conventions would suggest that this should be done internally
   // by createsongsoundsource, but this is not the convention in auxsnd.cpp
   m_pSndSrc->AddRef();

   return TRUE;
}

STDMETHODIMP_(void)
cSongPlayer::GetSong (ISong** ppSong)
{
   SONGPLR_TRACE(( "GetSong()" ));

   // Make alias.
   if (NULL != m_pSong)
      m_pSong->AddRef();
   *ppSong = m_pSong;
}

STDMETHODIMP_(void)
cSongPlayer::UnloadSong()
{
   SONGPLR_TRACE(( "UnloadSong()" ));

   if (! m_pSong)
      return;

   if (NULL != m_pStreamBuffer)
   {
      delete [] m_pStreamBuffer;
      m_pStreamBuffer = NULL;
   }

   uint numSegments = m_pSong->CountSections();
   for (int i = 0; i < numSegments; i++)
   {
      SafeRelease (m_pResArray[i]);
   }
   delete [] m_pResArray;
   m_pResArray = NULL;
   // we are not paused
   m_isPaused = FALSE;

   SafeRelease (m_pSndSrc);
   SafeRelease (m_pSndSample);
   SafeRelease (m_pSong);
}

STDMETHODIMP_(BOOL)
cSongPlayer::StartPlaying()
{
   SONGPLR_TRACE(( "StartPlaying()" ));

   if (NULL == m_pSndSample)
      return FALSE;

   if (m_isPlaying)
      return FALSE;

   // Set gate values.  The CurrSegGate is set to -1 as a debugging measure,
   // as it will be set upon the first segment callback.
   if ( m_bHaveMemento ) {
      // restore song player to save-game state
      m_pSndSrc->SetGate(kSNGPLR_CurrSegGate, m_memento.currSegGate );
      m_pSndSrc->SetGate(kSNGPLR_NextSegGate, m_memento.nextSegGate );
      m_pSndSrc->SetGate(kSNGPLR_LoopCountGate, m_memento.loopCountGate );
      if ( m_memento.lastBranch != ~0 ) {
         m_pSndSrc->BranchToLabel( m_memento.lastBranch );
      }
      // doh! lastEvent & lastTheme stuff may be screwed up, since setting
      //   the theme also generates a "set theme event"
      if ( m_memento.lastTheme[0] != 0 ) {
         SetTheme( m_memento.lastTheme );
      }
      if ( m_memento.lastEvent[0] != 0 ) {
         SendEvent( m_memento.lastEvent, m_memento.lastPriority );
      }
      m_bHaveMemento = FALSE;
   } else {
      m_pSndSrc->SetGate(kSNGPLR_CurrSegGate, ~0);
      m_pSndSrc->SetGate(kSNGPLR_NextSegGate, 0);
      m_pSndSrc->SetGate(kSNGPLR_LoopCountGate, 0);
   }

   m_pSndSample->SetVolume(m_volume);

   // @HACK: SFX faker.
   fake_sfx_hack hackola;
   hackola._dummy = 0;
   hackola._flags = FAKE_SFXFLG_HAPPY;
   m_pSndSample->SetData((long)&hackola);
   m_pSndSample->Play();
   // we are not paused
   m_isPaused = FALSE;

   if( !( hackola._flags & FAKE_SFXFLG_HAPPY ) )
   {
      return FALSE;
   }
   // End HACK

   m_isPlaying = TRUE;
   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongPlayer::StopPlaying()
{
   SONGPLR_TRACE(( "StopPlaying()" ));

   if (NULL == m_pSndSample)
      return FALSE;

   m_pSndSample->Stop();

   // we are not paused
   m_isPaused = FALSE;

   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongPlayer::PausePlaying()
{
   SONGPLR_TRACE(( "PausePlaying()" ));

   if (NULL == m_pSndSample)
      return FALSE;

   AssertMsg( !m_isPaused, "Redundant pause of song player" );
   m_isPaused = TRUE;

   m_pSndSample->Pause();

   return TRUE;
}

STDMETHODIMP_(BOOL)
cSongPlayer::ResumePlaying()
{
   SONGPLR_TRACE(( "ResumePlaying()" ));

   if (NULL == m_pSndSample)
      return FALSE;

   AssertMsg( m_isPaused, "Redundant resume of song player" );
   m_isPaused = FALSE;

   m_pSndSample->Resume();

   return TRUE;
}

STDMETHODIMP_(void)
cSongPlayer::SetVolume(int vol)
{
   SONGPLR_TRACE(( "SetVolume(%d)", vol ));

   m_volume = vol;

   if (NULL == m_pSndSample)
   {
      return;
   }

   m_pSndSample->SetVolume(vol);
}

STDMETHODIMP_(int)
cSongPlayer::GetVolume()
{
   SONGPLR_TRACE(( "GetVolume()" ));

   return m_volume;
   /*
   if (NULL == m_pSndSample)
   {
      return m_volume;
   }

   return (m_pSndSample->GetVolume());
   */
}

/*
cSongPlayer::SetEndCallback()
{
}
*/

STDMETHODIMP_(BOOL)
cSongPlayer::SendEvent(const char* pEventString, int priority)
{
   SONGPLR_TRACE(( "SendEvent(%s, %d)", pEventString, priority ));

   ISongSection* pSection;
   ISongEvent* pEvent;
   sSongEventInfo eventInfo;
   unsigned numEvents;
   int currSectionIndex;
   BOOL matched = FALSE;

   if (NULL == m_pSong)
   {
      Warning (("cSongPlayer::SendEvent() failed due to NULL m_pSong."));
      return FALSE;
   }

   // Do nothing if lower priority.
   if (priority < m_currPriority)
      return TRUE;

   if ( *pEventString ) {  // don't copy an empty event
      assert( strlen(pEventString) < sizeof(m_lastEvent) );
      // remember the last event recieved
      strcpy( m_lastEvent, pEventString );
      m_lastPriority = priority;
   }

   currSectionIndex = m_pSndSrc->GetGate (kSNGPLR_CurrSegGate);
   if (currSectionIndex == ~0)
   {
      Warning (("cSongPlayer::SendEvent(): Unexpected -1 value for CurrSegGate.\n"));
      return FALSE;
   }
   m_pSong->GetSection(currSectionIndex, &pSection);
   numEvents = pSection->CountEvents();

   // First, check for event in the section.
   for (unsigned i = 0 ; i < numEvents; i++)
   {
      pSection->GetEvent (i, &pEvent);
      pEvent->GetEventInfo (&eventInfo);
      if (! strcmp (eventInfo.eventString, pEventString))
      {
         _HandleEventMatch (pEvent);
         matched = TRUE;
         pEvent->Release();
         break;
      }
      pEvent->Release();
   }
   pSection->Release();

   // Check defaults (song-level events) if no match found.
   if (! matched)
   {
      numEvents = m_pSong->CountEvents();
      for (unsigned i = 0 ; i < numEvents; i++)
      {
         m_pSong->GetEvent (i, &pEvent);
         pEvent->GetEventInfo (&eventInfo);
         if (! strcmp (eventInfo.eventString, pEventString))
         {
            _HandleEventMatch (pEvent);
            pEvent->Release();
            break;
         }
         pEvent->Release();
      }
   }

   return TRUE;
}

STDMETHODIMP_(void)
cSongPlayer::SetThemed(BOOL isThemed)
{
   SONGPLR_TRACE(( "SetThemed(%d)", isThemed ));

   m_isThemed = isThemed;
}

STDMETHODIMP_(BOOL)
cSongPlayer::IsThemed()
{
   SONGPLR_TRACE(( "IsThemed()" ));

   return m_isThemed;
}

STDMETHODIMP_(BOOL)
cSongPlayer::SetTheme(const char* pThemeString)
{
   SONGPLR_TRACE(( "SetTheme(%s)", pThemeString ));
   // Set the theme string.
   strcpy (m_currTheme, pThemeString);

   // Send the event if themed.
   if (m_isThemed)
   {
      // Send the theme event.
      char themeStr[100];
      sprintf (themeStr, "theme %s", m_currTheme);
      SendEvent (themeStr, 0);
   }

   return TRUE;
}

//
// return a data chunk which is the song player state that
//   must be restored when a savegame is loaded
//
STDMETHODIMP_(void)
cSongPlayer::GetMemento( void  **pRetData,
                         int   *pRetSize )
{
   if (!m_pSndSrc)
   {
      *pRetData = 0;
      *pRetSize = 0;
      return;
   }

   m_memento.currSegGate = m_pSndSrc->GetGate( kSNGPLR_CurrSegGate );
   m_memento.nextSegGate = m_pSndSrc->GetGate( kSNGPLR_NextSegGate );
   m_memento.loopCountGate = m_pSndSrc->GetGate( kSNGPLR_LoopCountGate );
   m_memento.lastBranch = m_pSndSrc->GetMostRecentLabel();
   assert( strlen(m_lastEvent) < sizeof(m_memento.lastEvent) );
   strcpy( m_memento.lastEvent, m_lastEvent );
   m_memento.lastPriority = m_lastPriority;
   assert( strlen(m_currTheme) < sizeof(m_memento.lastTheme) );
   strcpy( m_memento.lastTheme, m_currTheme );

   *pRetData = &m_memento;
   *pRetSize = sizeof(sSongPlayerSaveState);
}


// return FALSE if data size mismatch
STDMETHODIMP_(BOOL)
cSongPlayer::RestoreMemento( void  *pData,
                             int   size )
{
   if ( sizeof(m_memento) == size ) {
      // restore the state
      memcpy( &m_memento, pData, size );
      // this flag tells StartPlaying to use the restored state
      m_bHaveMemento = TRUE;
      return TRUE;
   } else {
      return FALSE;
   }
}






void
cSongPlayer::_HandleEventMatch (ISongEvent* pEvent)
{
   SONGPLR_TRACE(( "_HandleEventMatch(EVENT)" ));

   ISongGoto* pGoto;
   sSongGotoInfo gotoInfo;

   // Generate a random number [0..99]
   //int randNum = (int) (RandFloat () * 100.0F);
   int randNum = UnrecordedRand() % 100;
   int totalProbability = 0;
   unsigned numGotos = pEvent->CountGotos();

   for (unsigned i = 0; i < numGotos; i++)
   {
      pEvent->GetGoto (i, &pGoto);
      pGoto->GetGotoInfo (&gotoInfo);
      pGoto->Release();

      totalProbability += gotoInfo.probability;

      // Choose this branch. Set the playlist gate so that it will branch appropriately.
      if (randNum < totalProbability)
      {
         m_pSndSrc->SetGate (kSNGPLR_NextSegGate, gotoInfo.sectionIndex);
         break;
      }
   }
}

void
cSongPlayer::_DoSegmentCallback()
{
   SONGPLR_TRACE(( "_DoSegmentCallback()" ));

   ISongSection* pSection;
   //ISongSample* pSample;
   //sSongSampleInfo sampleInfo;
   sSongSectionInfo sectionInfo;
   int gate;

   // If we get here and the loopcount gate is zero, then we just branched here
   // and need to set up stuff about the segment: the gates and the priority.
   if ( 0 == m_pSndSrc->GetGate(kSNGPLR_LoopCountGate) )
   {
      SONGPLR_SPEW(( "  Loopcount is zero: Resetting gates.\n" ));
      // Reset priority.
      m_currPriority = -1;

      // Update segment gates.
      gate = m_pSndSrc->GetGate(kSNGPLR_NextSegGate);
      m_pSndSrc->SetGate(kSNGPLR_CurrSegGate, gate);        // Move "next" into "current"
      m_pSndSrc->SetGate(kSNGPLR_NextSegGate, gate + 1);    // Default : "next" is next segment in sequence.

      // Reset loop count.
      m_pSong->GetSection(gate, &pSection);
      pSection->GetSectionInfo (&sectionInfo);
      /*
      pSection->GetSample(0, &pSample);
      pSample->GetSampleInfo(&sampleInfo); // @TBD : Move loopcount from sample to segment.
      pSample->Release();
      */
      pSection->Release();

      m_pSndSrc->SetGate(kSNGPLR_LoopCountGate, sectionInfo.loopCount);

      // Send null event to player.
      SendEvent ("", 0);

      // Send theme event if themed.
      if (m_isThemed)
      {
         char themeStr[100];
         sprintf (themeStr, "theme %s", m_currTheme);
         SendEvent (themeStr, 0);
      }
   }
}

void
cSongPlayer::_DoSongEndCallback()
{
   SONGPLR_TRACE(( "_DoSongEndCallback()" ));

   m_isPlaying = FALSE;

   UnloadSong();

   /*
   // Clean up.
   delete [] m_pStreamBuffer;
   m_pStreamBuffer = NULL;

   m_pSndSrc->Release();
   m_pSndSrc = NULL;

   m_pSndSample->Release();
   m_pSndSample = NULL;
   */
}

