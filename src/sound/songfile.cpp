/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songfile.cpp,v 1.2 1999/05/12 18:16:31 mwhite Exp $

// (MRW : Stolen from schfile.cpp)

#include <song.h>
#include <songfile.h>
#include <findhack.h>  // all the abstracted lg_find stuff
#include <hashset.h>
#include <hshsttem.h>
#include <str.h>
#include <datapath.h>
#include <config.h>
#include <cfgdbg.h>
#include <appagg.h>
#include <resapi.h>
#include <storeapi.h>
#include <binrstyp.h>

// Must be last header 
#include <dbmem.h>

#define SONG_FILEVERSION 1

#ifdef EDITOR
#define SONGFWRITE(f, x) fwrite (&(x), sizeof(x), 1, (f))
#endif // EDITOR

//#define SONGFREAD(f, x) fread (&(x), sizeof(x), 1, (f))
#define SONGREAD(p, x) { memcpy (&x, p, sizeof(x)); p += sizeof(x); }

//
// Everything but load (binary) is editor-only
//
#ifdef EDITOR

typedef struct sSongFile
{
   cStr fileName;
   cStr pathName;
} sSongFile;

class cSongFileNameHash: public cStrHashSet<sSongFile *>
{
private:
   tHashSetKey GetKey(tHashSetNode node) const;
};

tHashSetKey cSongFileNameHash::GetKey(tHashSetNode node) const
{
   return (tHashSetKey)((const char*)(((sSongFile*)node)->fileName));
}

static cSongFileNameHash fileNameHash;


static void SongFilesGet(const char *pszDataPath, const char *pszWhat)
{
   Datapath sDatapath;
   DatapathDir *pDatapathDir;
   char *pszFileName;
   sSongFile *pSongFile;

   DatapathClear(&sDatapath);
   DatapathAdd(&sDatapath, (char*)pszDataPath);
   pDatapathDir = DatapathOpenDir(&sDatapath, (char*)pszWhat, DP_SCREEN_DOT);
   while (NULL!=(pszFileName = DatapathReadDir(pDatapathDir)))
   {
      pSongFile = new sSongFile;
      pSongFile->fileName = pszFileName;
      (pSongFile->fileName).MakeLower();
      if (!fileNameHash.Search((const char*)(pSongFile->fileName)))
      {
         pSongFile->pathName = pszDataPath;
         fileNameHash.Insert(pSongFile);
      }
      else
         delete pSongFile;
   }
   DatapathCloseDir(pDatapathDir);
   DatapathFree(&sDatapath);
}

static void SongFilesRead(fSongRead readFunc)
{
   cStr fullName;
   sSongFile *pSongFile;
   tHashSetHandle handle;

   pSongFile = (sSongFile*)(fileNameHash.GetFirst(handle));
   while (pSongFile != NULL)
   {
      fullName.FmtStr("%s\\%s", (const char*)(pSongFile->pathName), 
                      (const char*)(pSongFile->fileName));
      fileNameHash.Remove(pSongFile);
      delete pSongFile;
      ConfigSpew("SongFiles", ("Loading file %s\n", (const char*)fullName));
      (*readFunc)((char*)((const char*)fullName));
      pSongFile = (sSongFile*)fileNameHash.GetFirst(handle);
   }
}


// read all files from res path and local dir that match "what" string
void SongFilesLoadFromDir(const char *where, const char *what, fSongRead readFunc)
{
   static char szDataPath[PATH_MAX];
   cStr resSongPath;

   // open stuff in your current dir
   resSongPath.FmtStr(".\\%s",where);
   SongFilesGet((const char*)resSongPath, what);

   // get stuff from res path
   if (config_get_raw("song_source_path", szDataPath, sizeof(szDataPath)))
   {
      resSongPath.FmtStr("%s\\%s", szDataPath, where);
      SongFilesGet((const char*)resSongPath, what);
   }
   // read it all
   SongFilesRead(readFunc);
}

//
// Song save/load
//
// These might be nicer if the objects themselves knew about how to save themselves.
// As it is, modifications to the binary format must be carefully matched in the
// SongSave and SongLoad.
void SongSave (ISong* pSong, char* filename)
{
   FILE* fp;

   ISongEvent* pEvent;
   ISongSection* pSection;
   ISongSample* pSample;
   ISongGoto* pGoto;

   sSongInfo songInfo;
   sSongEventInfo eventInfo;
   sSongSectionInfo sectionInfo;
   sSongSampleInfo sampleInfo;
   sSongGotoInfo gotoInfo;

   long fileVersion;

   unsigned count1, count2, count3, i, j, k;

   // Open file for writing.
   fp = fopen (filename, "wb");
   if (NULL == fp)
   {
      Warning (("Failure opening song file \"%s\" for writing.\n", filename));
      return;
   }

   // Write version number.
   fileVersion = SONG_FILEVERSION;
   SONGFWRITE (fp, fileVersion);

   // Write songInfo.
   pSong->GetSongInfo (&songInfo);
   SONGFWRITE (fp, songInfo);

   // Write events.
   count1 = pSong->CountEvents();
   SONGFWRITE (fp, count1);
   for (i = 0; i < count1; i++)
   {
      // Write event info.
      pSong->GetEvent(i, &pEvent);
      pEvent->GetEventInfo(&eventInfo);
      SONGFWRITE (fp, eventInfo);
      
      // Write gotos.
      count2 = pEvent->CountGotos();
      SONGFWRITE (fp, count2);
      for (j = 0; j < count2; j++)
      {
         // Write goto info.
         pEvent->GetGoto(j, &pGoto);
         pGoto->GetGotoInfo(&gotoInfo);
         SONGFWRITE (fp, gotoInfo);
         pGoto->Release();
      }
      pEvent->Release();
   }

   // Write sections.
   count1 = pSong->CountSections();
   SONGFWRITE (fp, count1);
   for (i = 0; i < count1; i++)
   {
      // Write section info.
      pSong->GetSection(i, &pSection);
      pSection->GetSectionInfo(&sectionInfo);
      SONGFWRITE (fp, sectionInfo);
      
      // Write samples.
      count2 = pSection->CountSamples();
      SONGFWRITE (fp, count2);
      for (j = 0; j < count2; j++)
      {
         // Write sample info.
         pSection->GetSample(j, &pSample);
         pSample->GetSampleInfo(&sampleInfo);
         SONGFWRITE (fp, sampleInfo);
         pSample->Release();
      }

      // Write Events.
      count2 = pSection->CountEvents();
      SONGFWRITE (fp, count2);
      for (j = 0; j < count2; j++)
      {
         // Write event info.
         pSection->GetEvent(j, &pEvent);
         pEvent->GetEventInfo(&eventInfo);
         SONGFWRITE (fp, eventInfo);
         
         // Write gotos.
         count3 = pEvent->CountGotos();
         SONGFWRITE (fp, count3);
         for (k = 0; k < count3; k++)
         {
            pEvent->GetGoto(k, &pGoto);
            pGoto->GetGotoInfo(&gotoInfo);
            SONGFWRITE (fp, gotoInfo);
            pGoto->Release();
         }
         pEvent->Release();
      }
      pSection->Release();
   }

   fclose (fp);
}
#endif // EDITOR

ISong* SongLoad (char* filename)
{
   // SongLoad loads a .snc file into memory using namedres and then
   // fits it into our data structures.
   ISearchPath* pSncPath;
   IRes* pRes;
   char *pData, *pDataStart;

   ISong* pSong;
   ISongEvent* pEvent;
   ISongSection* pSection;
   ISongSample* pSample;
   ISongGoto* pGoto;

   sSongInfo songInfo;
   sSongEventInfo eventInfo;
   sSongSectionInfo sectionInfo;
   sSongSampleInfo sampleInfo;
   sSongGotoInfo gotoInfo;

   long fileVersion;

   unsigned count1, count2, count3, i, j, k;

   //
   // Set up pSoundPath to point to the sound files
   //
   AutoAppIPtr(ResMan);
   pSncPath = pResMan->NewSearchPath();
   pSncPath->AddPathTrees("song\\", FALSE);
   pSncPath->Ready();

   pRes = pResMan->Bind (filename, RESTYPE_BINARY, pSncPath);
   if (NULL == pRes)
   {
      Warning (("Song \"%s\" not found.\n", filename));
      pSncPath->Release();
      return NULL;
   }

   pDataStart = (char*) pRes->Lock();
   pData = pDataStart;

   // Read version.
   SONGREAD (pData, fileVersion);
   // Simple version control : warn if not the expected version.
   if (fileVersion != SONG_FILEVERSION)
   {
      Warning (("Loading song \"%s\", file version is %d, expecting %d.\n",
                filename, fileVersion, (long) SONG_FILEVERSION));
   }

   // Read song.
   SONGREAD (pData, songInfo);
   CreateSong (&pSong, NULL);
   pSong->SetSongInfo(&songInfo);

   // Read events.
   SONGREAD (pData, count1);
   for (i = 0; i < count1; i++)
   {
      // Read event info.
      SONGREAD (pData, eventInfo);
      CreateSongEvent (&pEvent, NULL);
      pEvent->SetEventInfo(&eventInfo);

      // Read gotos.
      SONGREAD (pData, count2);
      for (j = 0; j < count2; j++)
      {
         // Read goto info.
         SONGREAD (pData, gotoInfo);
         CreateSongGoto (&pGoto, NULL);
         pGoto->SetGotoInfo(&gotoInfo);

         pEvent->AddGoto(pGoto);
         pGoto->Release();
      }
      pSong->AddEvent(pEvent);
      pEvent->Release();
   }

   // Read sections.
   SONGREAD (pData, count1);
   for (i = 0; i < count1; i++)
   {
      // Read section info.
      SONGREAD (pData, sectionInfo);
      CreateSongSection (&pSection, NULL);
      pSection->SetSectionInfo(&sectionInfo);

      // Read samples.
      SONGREAD (pData, count2);
      for (j = 0; j < count2; j++)
      {
         // Read sample info.
         SONGREAD (pData, sampleInfo);
         CreateSongSample (&pSample, NULL);
         pSample->SetSampleInfo(&sampleInfo);
         
         pSection->AddSample(pSample);
         pSample->Release();
      }

      // Read events.
      SONGREAD (pData, count2);
      for (j = 0; j < count2; j++)
      {
         // Read event info.
         SONGREAD (pData, eventInfo);
         CreateSongEvent (&pEvent, NULL);
         pEvent->SetEventInfo(&eventInfo);

         // Read gotos.
         SONGREAD (pData, count3);
         for (k = 0; k < count3; k++)
         {
            // Read goto info.
            SONGREAD (pData, gotoInfo);
            CreateSongGoto (&pGoto, NULL);
            pGoto->SetGotoInfo(&gotoInfo);

            pEvent->AddGoto(pGoto);
            pGoto->Release();
         }
         pSection->AddEvent(pEvent);
         pEvent->Release();
      }

      pSong->AddSection(pSection);
      pSection->Release();
   }

   pRes->Unlock();
   pRes->Release();
   pSncPath->Release();

   return pSong;
}
