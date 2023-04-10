/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/backup.cpp,v 1.11 1998/09/28 13:25:03 CMONTER Exp $ 
#include <backup.h>
#include <io.h>

#include <dbfile.h>
#include <dbasemsg.h>
#include <config.h>
#include <cfgtool.h>

#include <texsave.h>

#include <tagfile.h>
#include <tFileFac.h>
#include <memtfile.h>

#include <dbmem.h>

EXTERN BOOL wr_db_disabled;

#define BACKUPFILE "togame.cow" 

static cTagFileFactory* factory = NULL; 

static BOOL saving =FALSE; 

void BackupMission(void)
{
   ITagFile* file = NULL; 


   int use_mem = 1; 
   config_get_int("game_mem_backup",&use_mem);

   if (use_mem)
   {
      delete factory;
      factory = CreateMemoryTagFileFactory();
      file = factory->Open(BACKUPFILE,kTagOpenWrite);
   }
   else
      file = TagFileOpen(BACKUPFILE,kTagOpenWrite);

   wr_db_disabled = !config_is_defined("backup_wr"); 
   saving = TRUE; 

   dbSaveTagFile(file,kFiletypeAll);

   SafeRelease(file); 

   wr_db_disabled = FALSE; 
   saving = FALSE; 

}

static BOOL restoring = FALSE;

void RestoreMissionBackup(void)
{
   int do_backup = 1;
   config_get_int("game_mode_backup",&do_backup); 
   if (do_backup) 
   {
      ITagFile* file = NULL; 
      if (factory)
         file = factory->Open(BACKUPFILE,kTagOpenRead);
      else
         file = TagFileOpen(BACKUPFILE,kTagOpenRead);

      if (file != NULL) 
      {
         wr_db_disabled = TRUE; 
         texture_EnableSaveLoad(FALSE);

         restoring = TRUE; 

         dbLoadTagFile(file,kFiletypeAll); 

         SafeRelease(file);

         restoring = FALSE; 
         texture_EnableSaveLoad(TRUE);

         wr_db_disabled = FALSE; 

      }

   }


}

void RemoveMissionBackup(void)
{
   if (factory)
   {
      delete factory; 
      factory = NULL;
   }
   remove(BACKUPFILE);
}

BOOL BackupAvailable(void)
{
   if (factory) return TRUE; 
   char buf[256]; 
   return dbFind(BACKUPFILE,buf); 
}

BOOL BackupLoading(void)
{
   return restoring; 
}

BOOL BackupSaving(void)
{
   return saving; 
}


void InitBackup(void)
{
   // If there's a backup file, we probably don't want it
   remove(BACKUPFILE);
}

void TermBackup(void)
{
   RemoveMissionBackup(); 
}


