/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songedtr.cpp,v 1.4 2000/02/19 12:37:15 toml Exp $

#include <songedtr.h>
#include <song.h>
#include <songplr.h>
#include <songutil.h>

#include <command.h>
#include <songfile.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Compiled into editor builds only: no need for #ifdef EDITOR.
//

extern "C" {
BOOL SongYaccParse (char* songFile, ISong **ppSong);
}

static void _CmdRebuildSongs ();
//static void _CmdParseLoadSong (char* pFilename);
static void _CmdLoadSong (char* pFilename);
static void _CmdPlaySong ();
static void _CmdStopSong ();
//static void _CmdUnloadSong ();
static void _CmdSongEvent (char* pEventString);
#ifndef SHIP
static void _CmdSongDumpMono ();
#endif

//static void _SongDumpToMono(ISong* pSong);
static void _SongParseAndSave (char* filename);
static void _SongSetupCommands();

//
// Static globals.
//
static Command sgCommands[] =
{
   { "rebuild_songs", FUNC_VOID, _CmdRebuildSongs, "Reparse all songs into compiled format", HK_ALL },
   //{ "parseload_song", FUNC_STRING, _CmdParseLoadSong, "Load a song song by parsing it.", HK_ALL },
   { "load_song", FUNC_STRING, _CmdLoadSong, "Load a song", HK_ALL },
   { "play_song", FUNC_VOID, _CmdPlaySong, "Play the loaded song", HK_ALL},
   { "stop_song", FUNC_VOID, _CmdStopSong, "Stop the currently playing song", HK_ALL},
   //{ "unload_song", FUNC_VOID, _CmdUnloadSong, "Unload song.", HK_ALL},
   { "song_event", FUNC_STRING, _CmdSongEvent, "Send an event to the currently playing song", HK_ALL},
#ifndef SHIP
   { "song_dumpmono", FUNC_VOID, _CmdSongDumpMono, "Dump info about current song to mono", HK_ALL}
#endif
};

//ISong* sgpSong;
//static ISongPlayer* sgpSongPlayer;

//ISndMixer* sgpSndMixer;
//ISearchPath* sgpWavPath;

// Init/Shutdown
BOOL SongEd_Init ()//ISongPlayer* pSongPlayer)
{
   _SongSetupCommands();

   /*
   if (NULL == pSongPlayer)
   {
      Warning (("pSongPlayer was NULL in SongEd_Init.\n"));
      return FALSE;
   }

   // Make an alias of the player.
   pSongPlayer->AddRef();
   sgpSongPlayer = pSongPlayer;
   */

   return TRUE;
}

void SongEd_Shutdown ()
{
   //SafeRelease(sgpSongPlayer);
}

//
// Command callbacks.
//

static void _CmdRebuildSongs ()
{
   SongFilesLoadFromDir("songsrc", "*.sns", _SongParseAndSave);
}

/*
static void _CmdParseLoadSong (char* pFilename)
{
  
   SafeRelease(sgpSong);

   SongYaccParse (pFilename, &sgpSong);
   _SongDumpToMono (sgpSong);
  
}
*/

static void _CmdLoadSong (char* pFilename)
{
   SongUtilLoad (pFilename);
   /*
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
   */
}

static void _CmdPlaySong ()
{
   SongUtilPlay();
   /*
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot play song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   sgpSongPlayer->StartPlaying();
   */
}

static void _CmdStopSong ()
{
   SongUtilStop();
   /*
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot stop song.  sgpSongPlayer is NULL.\n"));
      return;
   }

   // @TBD : Implement.
   //   Warning (("_CmdStopSong() not implemented."));
   sgpSongPlayer->StopPlaying();
   */
}

/*
static void _CmdUnloadSong ()
{
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot unload song.  sgpSongPlayer is NULL.\n"));
      return;
   }
   // @TBD : Either implement or get rid of this.
   Warning (("_CmdUnloadSong() not implemented."));
}
*/

static void _CmdSongEvent (char* pEventString)
{
   SongUtilEvent (pEventString);
   /*
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot send song event.  sgpSongPlayer is NULL.\n"));
      return;
   }

   sgpSongPlayer->SendEvent(pEventString, 1);
   */
}

#ifndef SHIP
static void _CmdSongDumpMono ()
{
   SongUtilDumpMono ();
   /*
   if (NULL == sgpSongPlayer)
   {
      Warning (("Cannot dump song to mono.  sgpSongPlayer is NULL.\n"));
      return;
   }

   ISong* pSong;

   sgpSongPlayer->GetSong (&pSong);
   _SongDumpToMono (pSong);
   SafeRelease (pSong);
   */
}
#endif

//
//
//

static void _SongSetupCommands()
{
   static BOOL sbCommandsSetup;

   if (! sbCommandsSetup)
   {
      COMMANDS(sgCommands, HK_ALL);
      sbCommandsSetup = TRUE;
   }
}

static void _SongParseAndSave (char* filename)
{
   ISong* pSong;
   BOOL isOK;
   char outFilename[100];
   int nopathIndex, len, dotIndex, i;

   isOK = SongYaccParse (filename, &pSong);
   
   if (isOK)
   {
      // String hacking in progress...
      // Pull out path of filename.
      // Find last backslash.
      nopathIndex = 0;
      len = strlen (filename);   
      for (i = 0; i < len; i++)
         if (filename[i] == '\\')
            nopathIndex = i + 1;
      sprintf(outFilename, "song\\%s", &(filename[nopathIndex]));
      // Change extension...
      len = strlen (outFilename);
      // Note: Default to appending extension if no dot found.
      dotIndex = len;
      // Find last dot in filename.
      for (i = 0; i < len; i++)
         if (outFilename[i] == '.')
            dotIndex = i;
      // Append new extension where old one was.
      outFilename[dotIndex] = '\0';
      strcat (outFilename, ".snc"); // snc = "SoNg, Compiled"

      // Save and release the song.
      SongSave (pSong, outFilename);
      pSong->Release();
   }
   else
   {
      Warning (("Failed to parse %s.\n", filename));
   }
}
