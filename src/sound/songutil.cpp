/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songutil.cpp,v 1.13 2000/02/29 17:44:44 patmac Exp $

#include <songutil.h>
#include <song.h>
#include <songplr.h>
#include <lgsound.h>
#include <mprintf.h>
#include <appagg.h>
#include <resapi.h>
#include <songmiss.h>
#include <songfile.h>
#include <filevar.h>
#include <dbasemsg.h>

/*
#ifdef EDITOR
#include <songedtr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
#endif
*/

//
// Static globals.
//

//static ISong* sgpSong;
//static int sgVolume;
static BOOL sgDisabled;
static ISongPlayer* sgpSongPlayer;
static ISndMixer* sgpSndMixer;
static ISearchPath* sgpWavPath;
static int sgpSoundGroup = 0;

// Filvar stuff for save/load song theme.

// Here's the type of my global 
struct sThemeSaveLoad 
{
   char themeName[32];
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gThemeVarDesc = 
{
   kCampaignVar,        // Where do I get saved?
   "SONGTHEME",         // Tag file tag
   "Song Theme",        // friendly name
   FILEVAR_TYPE(sThemeSaveLoad),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   //TBD: shouldn't this be NULL for all games ? - patmc
   "shock",             // optional: what game am I in NULL means all 
}; 

// The actual global variable
static cFileVar<sThemeSaveLoad,&gThemeVarDesc> sgThemeSaveLoad; 


// Here's my descriptor, which identifies my stuff to the tag file & editor
static TagVersion SongMementoVersion = {1, 0};
static TagFileTag SongMementoTag = {"SNGMEMENTO"};

//
// Exported functions.
//
void SongUtilInit (ISndMixer* pSndMixer)
{
   //sgpSong = NULL;
   sgpSongPlayer = NULL;
   sgpSndMixer = NULL;
   sgpWavPath = NULL;

   // Set up pSoundPath to point to the sound files
   AutoAppIPtr(ResMan);
   sgpWavPath = pResMan->NewSearchPath();
   if (NULL == sgpWavPath)
   {
      Warning (("SongUtil_Init() could not create a search path.\n"));
      return;
   }
   sgpWavPath->AddPathTrees("snd\\", FALSE);
   sgpWavPath->Ready();

   // Alias the mixer.
   pSndMixer->AddRef();
   sgpSndMixer = pSndMixer;

   // Create the song player.
   CreateSongPlayer (&sgpSongPlayer, sgpSndMixer, sgpWavPath, NULL, sgpSoundGroup);

   sgpSongPlayer->SetThemed (TRUE);

   MissionSongInit();

   sgDisabled = FALSE;
   /*
#ifdef EDITOR
   SongEd_Init (sgpSongPlayer);
#endif // EDITOR
   */
}

void SongUtilShutdown ()
{
   /*
#ifdef EDITOR
   SongEd_Shutdown();
#endif
   */

   MissionSongTerm();

   SafeRelease (sgpSongPlayer);
   SafeRelease (sgpSndMixer);
   SafeRelease (sgpWavPath);
}

void SongUtilDisableMusic ()
{
   if( !sgDisabled )
   {
      sgDisabled = TRUE;
   }
}

void SongUtilEnableMusic ()
{
   if( sgDisabled )
   {
      sgDisabled = FALSE;
   }
}

void SongUtilSetTheme (const char* pThemeStr)
{
   if (!sgpSongPlayer)
      return;

   strcpy( sgThemeSaveLoad.themeName, pThemeStr );

   if( !sgDisabled )
      sgpSongPlayer->SetTheme(pThemeStr);
}

void SongUtilLoad (const char* pFilename)
{
   char extendedFilename[100];
   ISong* pSong;

   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot load song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   strcpy (extendedFilename, pFilename);
   strcat (extendedFilename, ".snc");

   pSong = SongLoad (extendedFilename);
   if (NULL != pSong)
   {
      sgpSongPlayer->SetSong (pSong);
      pSong->Release();
   }
   else
   {
      Warning (("Could not load song \"%s\".\n", extendedFilename));
   }
}

void SongUtilPlay ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot play song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   if( !sgDisabled )
   {
      sgpSongPlayer->StartPlaying();
      sgpSongPlayer->SetTheme( sgThemeSaveLoad.themeName );
   }
}

void SongUtilStop ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot stop song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   if( !sgDisabled )
      sgpSongPlayer->StopPlaying();
}

void SongUtilPause ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot pause song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   if( !sgDisabled )
      sgpSongPlayer->PausePlaying();
}

void SongUtilResume ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot resume song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   if( !sgDisabled )
      sgpSongPlayer->ResumePlaying();
}

void SongUtilSetVolume (int vol)
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot set volume on song.  sgpSongPLayer is NULL.\n"));
      return;
   }

   sgpSongPlayer->SetVolume(vol);
}

int SongUtilGetVolume ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot get volume on song.  sgpSongPLayer is NULL.\n"));
      return 0;
   }

   return ( sgpSongPlayer->GetVolume() );
}

void SongUtilEvent (const char *pEventString)
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot send song event.  sgpSongPlayer is NULL.\n"));
      return;
   }

   if( !sgDisabled )
      sgpSongPlayer->SendEvent(pEventString, 1);
}


/////////////////////////////////////////////////////////////
//
//  MISSION LOAD/SAVE STUFF
//

BOOL
SongUtilLoadMission(ITagFile *file)
{
   TagVersion found_version = SongMementoVersion;
   HRESULT result = ITagFile_OpenBlock( file, &SongMementoTag, &found_version );
   BOOL rv = FALSE;
   void *pBlock;
   int blockSize;

   // Hack! ignore the minor version so both 1.1 & 1.0 will work
   //  1.0 saves are just missing the last event field
   if ( (result == S_OK)
        && (found_version.major == SongMementoVersion.major)
        && (found_version.minor == SongMementoVersion.minor) ) {

      blockSize = ITagFile_BlockSize( file, &SongMementoTag );
      pBlock = Malloc( blockSize );
      ITagFile_Read( file, (char *) pBlock, blockSize );
      ITagFile_CloseBlock(file);

      if ( sgpSongPlayer && !sgDisabled ) {
         sgpSongPlayer->RestoreMemento( pBlock, blockSize );
      }

      Free( pBlock );
   }
   return rv;
}

BOOL
SongUtilSaveMission(ITagFile *file)
{
   HRESULT result;
   BOOL rv = FALSE;
   void *pMemento;
   int  mementoSize;

   if ( sgpSongPlayer == NULL ) {
      return FALSE;
   }
   sgpSongPlayer->GetMemento( &pMemento, &mementoSize );

   if ( mementoSize ) {
      result = ITagFile_OpenBlock(file, &SongMementoTag, &SongMementoVersion);
      if (result == S_OK) {
         ITagFile_Write(file, (char *) pMemento, mementoSize );
         ITagFile_CloseBlock(file);
      }
   }
   return rv;
}


void
SongUtilResetMission()
{
   sgThemeSaveLoad.themeName[0] = 0;
}

void SongUtilSetSoundGroup( int songGroup )
{
   sgpSoundGroup = songGroup;
}

/////////////////////////////////////////////////////////////
// DEBUGGING FUNCTIONS
//

#ifndef SHIP
static void _SongDumpToMono(ISong* pSong);

void SongUtilDumpMono ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot dump song to mono.  sgpSongPlayer is NULL.\n"));
      return;
   }

   ISong* pSong;

   sgpSongPlayer->GetSong (&pSong);
   _SongDumpToMono (pSong);
   SafeRelease (pSong);
}

static void _SongDumpToMono(ISong* pSong)
{
   ISongSection* pSection;
   ISongSample* pSample;
   ISongEvent* pEvent;
   ISongGoto* pGoto;

   sSongInfo songInfo;
   sSongSectionInfo sectionInfo;
   sSongSampleInfo sampleInfo;
   sSongEventInfo eventInfo;
   sSongGotoInfo gotoInfo;

   unsigned sectionCount, sampleCount, eventCount, gotoCount;
   unsigned i, j, k;

   if (NULL == pSong)
   {
      Warning (("Song was NULL in _SongDumpToMono()\n"));
      return;
   }

   // Spew song info.
   pSong->GetSongInfo(&songInfo);
   eventCount = pSong->CountEvents();
   sectionCount = pSong->CountSections();
   mprintf ("Song %s: events = %d, sections = %d.\n", songInfo.id, eventCount, sectionCount);

   // Spew default event info.
   for (i = 0; i < eventCount; i++)
   {
      pSong->GetEvent(i, &pEvent);
      pEvent->GetEventInfo(&eventInfo);
      gotoCount = pEvent->CountGotos();
      mprintf ("  Event \"%s\": flags = %d, gotos = %d.\n",
               eventInfo.eventString, eventInfo.flags, gotoCount);

      // Spew goto info.
      for (k = 0; k < gotoCount; k++)
      {
         pEvent->GetGoto(k, &pGoto);
         pGoto->GetGotoInfo(&gotoInfo);
         pGoto->Release();
         mprintf ("    Goto : sectionIndex = %d, probability = %d.\n",
                  gotoInfo.sectionIndex, gotoInfo.probability);
      }
      pEvent->Release();
   }

   // Spew section info.
   for (i = 0; i < sectionCount; i++)
   {
      pSong->GetSection(i, &pSection);
      pSection->GetSectionInfo(&sectionInfo);
      sampleCount = pSection->CountSamples();
      eventCount = pSection->CountEvents();
      mprintf ("  Section %s: volume = %d, loopCount = %d, samples = %d, events = %d.\n",
               sectionInfo.id, sectionInfo.volume, sectionInfo.loopCount, sampleCount, eventCount);

      // Spew sample info.
      for (j = 0; j < sampleCount; j++)
      {
         pSection->GetSample(j, &pSample);
         pSample->GetSampleInfo(&sampleInfo);
         pSample->Release();
         mprintf ("    Sample \"%s\"\n",//: loopCount = %d, probability = %d.\n",
                  sampleInfo.name);//, sampleInfo.loopCount, sampleInfo.probability);
      }

      // Spew event info.
      for (j = 0; j < eventCount; j++)
      {
         pSection->GetEvent(j, &pEvent);
         pEvent->GetEventInfo(&eventInfo);
         gotoCount = pEvent->CountGotos();
         mprintf ("    Event \"%s\": flags = %d, gotos = %d.\n",
                  eventInfo.eventString, eventInfo.flags, gotoCount);

         // Spew goto info.
         for (k = 0; k < gotoCount; k++)
         {
            pEvent->GetGoto(k, &pGoto);
            pGoto->GetGotoInfo(&gotoInfo);
            pGoto->Release();
            mprintf ("      Goto : sectionIndex = %d, probability = %d.\n",
                     gotoInfo.sectionIndex, gotoInfo.probability);
         }
         pEvent->Release();
      }
      pSection->Release();
   }
}
#endif

