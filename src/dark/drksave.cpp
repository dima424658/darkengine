/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksave.cpp,v 1.16 2000/03/21 15:57:09 adurant Exp $
#include <appagg.h>
#include <drksave.h>
#include <tagfile.h>
#include <vernum.h>

#include <string.h>

#include <campaign.h>
#include <quesfile.h>

#include <dbfile.h>
#include <dbtagfil.h>
#include <dbasemsg.h>
#include <dispatch.h>
#include <dispbase.h>


#include <simman.h>
#include <simdef.h>

#include <drkmislp.h>
#include <drkmiss.h>
#include <drkdebrf.h>
#include <drkamap.h>

#include <ctype.h>

// For quick save 
#include <drkuires.h>
#include <drkmsg.h>
#include <direct.h>

// For checkpoints
#include <direct.h>
#include <stdio.h>

#include <command.h>
#include <config.h>

#include <mprintf.h>

#include <btffact.h>

// must be last header
#include <dbmem.h>


//----------------------------------------
// Save game descriptor tag 
//

static TagFileTag desc_tag = { "SAVEDESC" }; 
static TagVersion desc_ver = { 3, 3 };

enum eSaveGameFlags
{
   kInMission  = 1 << 0, 
}; 

struct sSaveGameDesc 
{
   char text[1024]; // string description
   ulong flags;
}; 

//------------------------------------------------------------
// SAVE THE GAME
//

HRESULT DarkSaveGameInternal(const char* descstr, ITagFile* file)
{
   AutoAppIPtr_(SimManager,pSimMan); 
   ulong lastmsg = pSimMan->LastMsg(); 
   // we are "in the sim" if we can't change difficulty
   BOOL in_sim = !CanChangeDifficultyNow(); 

   // build the descriptor 
   sSaveGameDesc desc = { "", 0 }; 
   strncpy(desc.text,descstr,sizeof(desc.text)); 
   if (in_sim) 
      desc.flags = kInMission; 

   //
   // write the descriptor
   //
   TagVersion v = desc_ver; 
   if (FAILED(file->OpenBlock(&desc_tag,&v)))
      return kDarkSaveBlockOpen; 
   
   int len = file->Write((char*)&desc,sizeof(desc)); 

   if (FAILED(file->CloseBlock()))
      return kDarkSaveBlockClose; 

   if (len != sizeof(desc))
      return kDarkSaveReadWrite; 


   //
   // Write the campaign
   //

   AutoAppIPtr(Campaign); 
   if (FAILED(pCampaign->Save(file)))
      return kDarkSaveCampaign;

   // 
   // Write the mission 
   //

   if (in_sim)
   {
      if (FAILED(dbSaveTagFile(file,kFiletypeMIS|kObjPartTerrain)))
         return kDarkSaveMission;
   }
   else
   {
      // Save the mission quest data for debrief purposes
      QuestDataSaveTagFile(kQuestDataMission,file); 
      SaveMissionData(file); 
      SaveMapSourceInfo(file);
   }

   return S_OK; 
   
}

static TagFileTag checkpoint_tag = { "MISSTART" }; 
static TagVersion checkpoint_ver = { 1, 0 } ;

HRESULT DarkSaveGame(const char* descstr, ITagFile* file)
{
   HRESULT result = DarkSaveGameInternal(descstr,file); 
   if (FAILED(result))
      return result; 

   const char* fname = CHECKPOINT_SAVE; 
   // pack the checkpoint into the saved game
   FILE* cp = fopen(fname,"rb"); 
   if (!cp)
      return kDarkSaveCheckpoint;

   TagVersion v = checkpoint_ver; 
   if (FAILED(file->OpenBlock(&checkpoint_tag,&v)))
      return kDarkSaveBlockOpen; 
   
   // copy the file in 4k blocks 
   char buf[4096]; 
   while (!feof(cp))
   {
      int len = fread(buf,1,sizeof(buf),cp); 
      if (file->Write(buf,len) != len)
         return kDarkSaveCheckpoint; 
   }

   fclose(cp); 

   if (FAILED(file->CloseBlock()))
      return kDarkSaveBlockClose; 

   return result; 
}


//
// Save to a filename
//



long DarkSaveGameFile(const char* desc, const char* fname)
{
   ITagFile* file = TagFileOpen(fname,kTagOpenWrite); 
   long retval = DarkSaveGame(desc,file); 
   SafeRelease(file); 
   return retval; 
}


//------------------------------------------------------------
// LOAD THE GAME 
//

static inline BOOL partially_invalid(const TagVersion& v)
{
   if (config_is_defined("load_old_saves"))
      return FALSE; 
   return VersionNumsCompare(&v,&desc_ver) < 0; 
}

static inline BOOL fully_invalid(const TagVersion& v)
{
   if (config_is_defined("load_old_saves"))
      return FALSE; 
   return v.major < desc_ver.major; 
}

HRESULT DarkLoadGameInternal(ITagFile* file)
{
   
   //
   // Read the descriptor
   //
   sSaveGameDesc desc; 
   TagVersion v = desc_ver; 
   if (FAILED(file->OpenBlock(&desc_tag,&v)))
      return kDarkSaveBlockOpen; 

   int len = file->Read((char*)&desc,sizeof(desc)); 

   if (FAILED(file->CloseBlock()))
      return kDarkSaveBlockClose; 

   if (len != sizeof(desc))
      return kDarkSaveReadWrite; 

   // Invalidate save games
   if (fully_invalid(v))
      return kDarkSaveVersionInvalid; 

   // if minor version is old, then convert to an in-mission save

   if (partially_invalid(v))
      desc.flags &= ~kInMission; 

   //
   // Stop the sim and reset the database 
   //
   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->StopSim(); 
   dbReset(); 

   //
   // Read the campaign
   //

   AutoAppIPtr(Campaign); 
   if (FAILED(pCampaign->Load(file)))
      return kDarkSaveCampaign;

   // 
   // Read the mission 
   //

   // get the mission number and path
   LoadMissionData(file); 
   LoadMapSourceInfo(file);

#ifdef DEMO
   if (GetMissionData()->num>2)
      return kDarkSaveVersionInvalid; 
#endif

   if (desc.flags & kInMission)
   {
      // load the mission file 
      ITagFile* missfile = OpenMissionFile(GetMissionData()->num); 

      // Load the map (but not map objects) from the virgin mission file
      if (FAILED(dbMergeLoadTagFile(missfile,kFiletypeGAM|kDBMap)))
         return kDarkSaveMission; 

      SafeRelease(missfile); 

      // Now merge-load the mission (& all concretes) from the save game 
      if (FAILED(dbMergeLoadTagFile(file,kFiletypeMIS|kObjPartTerrain)))
         return kDarkSaveMission;

      // send the final post load
      DispatchData msg = { kDatabasePostLoad, 0 }; 
      DispatchMsg(kMsgDatabase,&msg); 

      // merge load doesn't suspend the sim, so we do it ourselves.
      pSimMan->SuspendSim(); 
   }
   else
   {
      // Load the mission quest data for debrief purposes
      QuestDataLoadTagFile(kQuestDataMission,file); 
      LoadMissionData(file); 
      LoadMapSourceInfo(file);
   }

   // if partially invalid, return to the start of the mission loop 
   if (partially_invalid(v) || config_is_defined("restart_on_load"))
      MissionLoopReset(kMissLoopRestartMission); 

   return S_OK; 
}

//----------------------------------------


HRESULT DarkLoadGame(ITagFile* file)
{

   TagVersion v = checkpoint_ver; 
   if (SUCCEEDED(file->OpenBlock(&checkpoint_tag,&v)))
   {
      // unpack the checkpoint from the saved game
      FILE* cp = fopen(CHECKPOINT_SAVE,"wb"); 
      if (!cp)
         return kDarkSaveBlockOpen; 
   
      // copy the file in 4k blocks 
      char buf[4096]; 
      while (file->TellFromEnd() > 0)
      {
         int len = file->Read(buf,sizeof(buf));

         if (fwrite(buf,1,len,cp) != len)
            return kDarkSaveCheckpoint; 
      }

      fclose(cp); 

      if (FAILED(file->CloseBlock()))
         return kDarkSaveBlockClose; 
   }

   HRESULT result = DarkLoadGameInternal(file); 
   if (FAILED(result))
      return result; 


   return result; 
}


//
// Load a file 
//
long DarkLoadGameFile(const char* fname)
{
   ITagFile* file = BufTagFileOpen(fname,kTagOpenRead); 
   if (!file)
   {
#ifndef SHIP
      mprintf("%s not found.\n", fname);
#endif
      return E_FAIL;
   }
      
   long result = DarkLoadGame(file); 
   SafeRelease(file); 
   return result; 
}


//------------------------------------------------------------
// Extract info from a file
//

BOOL DarkLoadGameDescription(ITagFile* file, char* buf, int buflen)
{
   Assert_(buf && buflen > 0); 
   *buf = '\0'; 

   TagVersion v = desc_ver; 
   if (FAILED(file->OpenBlock(&desc_tag,&v)))
      return FALSE; 

   if (fully_invalid(v))
   {
      file->CloseBlock(); 
      return FALSE;
   }

   sSaveGameDesc desc; 
   int len = file->Read((char*)&desc,sizeof(desc)); 

   file->CloseBlock(); 

   if (len != sizeof(desc))
      return FALSE; 

   strncpy(buf,desc.text,buflen); 
   
   return TRUE; 
}

BOOL DarkGameIsMidMission(ITagFile* file)
{
   TagVersion v = desc_ver; 
   if (FAILED(file->OpenBlock(&desc_tag,&v)))
      return FALSE; 

   if (partially_invalid(v))
   {
      file->CloseBlock(); 
      return FALSE;
   }

   sSaveGameDesc desc; 
   int len = file->Read((char*)&desc,sizeof(desc)); 

   file->CloseBlock(); 

   if (len != sizeof(desc))
      return FALSE; 

   return (desc.flags & kInMission) != 0; 
}


//------------------------------------------------------------
// Quick Save
//

void DarkQuickSaveGame(void)
{
   mkdir(SAVE_PATH); 
   cStr desc = FetchUIString("gamesav","quick_save"); 
   BOOL success = SUCCEEDED(DarkSaveGameFile(desc,SAVE_PATH "\\" QUICK_SAVE)); 
   char* msg = (success) ? "success" : "failure"; 
   DarkMessage(FetchUIString("gamesav",msg)); 
}

void DarkQuickLoadGame(void)
{
   long success =   DarkQuickLoadGameFile(SAVE_PATH "\\" QUICK_SAVE); 

   switch(success)
   {
      case kQuickLoadNoFile:
         DarkMessage(FetchUIString("gamelod","failed")); 
         break; 

      case kQuickLoadFailed:
         MissionLoopReset(kMissLoopMainMenu); 
         UnwindToMissionLoop(); 
         break; 
   }
}

long DarkQuickLoadGameFile(const char* filename)

{
   ITagFile* file = BufTagFileOpen(filename,kTagOpenRead); 
   
   if (!file)
      return kQuickLoadNoFile; 

   long retval = kQuickLoadFailed; 
   

   if (DarkGameIsMidMission(file))
   {
      PushToSaveGameLoadingMode(file); 
      retval = S_OK; 
   }
   else
   {
      retval = DarkLoadGame(file); 
      
      if (SUCCEEDED(retval))
         UnwindToMissionLoop(); 
      else
         retval = kQuickLoadFailed; 
   }


   SafeRelease(file);
   return retval; 
}

//------------------------------------------------------------
// Checkpoint for restore
//

EXTERN void DarkSaveCheckpoint(void)
{
   ITagFile* file = TagFileOpen(CHECKPOINT_SAVE,kTagOpenWrite); 
   long retval = DarkSaveGameInternal("Mission Start",file); 
   SafeRelease(file); 
}

EXTERN void DarkRestoreCheckpoint(void)
{
   ITagFile* file = BufTagFileOpen(CHECKPOINT_SAVE,kTagOpenRead); 
   long retval = DarkLoadGameInternal(file); 
   SafeRelease(file); 
}


//------------------------------------------------------------
// COMMANDS
//

#ifdef EDITOR
static void edit_load_game(char* arg)
{
   // strip trailing whitespace
   for (char* s = arg+strlen(arg)-1; s >= arg && isspace(*s); s--)
      *s = '\0';

   DarkLoadGameFile(arg); 
}; 

static void edit_save_game(char* arg)
{
   // strip trailing whitespace
   for (char* s = arg+strlen(arg)-1; s >= arg && isspace(*s); s--)
      *s = '\0';

   DarkSaveGameFile("From Editor",arg); 
}; 
#endif // EDITOR

static Command commands[] = 
{
   { "quick_save", FUNC_VOID, DarkQuickSaveGame, "Save the game to the 'quick save' slot.", HK_GAME_MODE },
   { "quick_load", FUNC_VOID, DarkQuickLoadGame, "Load the game from the 'quick save' slot", HK_GAME_MODE }, 

#ifdef EDITOR
   { "edit_load_game", FUNC_STRING, edit_load_game, "Load a save game in editor", HK_EDITOR },
   { "edit_save_game", FUNC_STRING, edit_save_game, "Save a save game in editor", HK_EDITOR },
#endif 
}; 


//------------------------------------------------------------
// INIT/TERM
//

void DarkSaveGameInit(void)
{
   COMMANDS(commands,HK_ALL); 
}

void DarkSaveGameTerm(void)
{
}
