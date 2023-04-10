/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motmngr.cpp,v 1.20 1999/05/27 13:59:23 JON Exp $

#ifndef SHIP
//#define PROFILE_ON
#endif

#include <motmngr.h>
#include <motset.h>
#include <motdb.h>
#include <command.h>
#include <aiapi.h>
#include <motprop.h>
#include <appagg.h>
#include <propbase.h>
#include <propman.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <cfgtool.h>
#include <timings.h>

#include <dbmem.h> // must be last included header

DECLARE_TIMER(MOTMNGR_PF_LOAD, Average);

IMotionSet *g_pMotionSet=NULL;

cMotionDatabase *g_pMotionDatabase=NULL;

/// GLOBALS
cManeuverFactoryList g_ManeuverFactoryList;

static TagFileTag g_MotDBTag = { "MotDBase" };
static VersionNum g_MotDBVersion={1,8};

#ifdef EDITOR
static void BuildMotionDatabase()
{
   AutoAppIPtr(AIManager);
   Assert_(g_pMotionDatabase);
   Assert_(g_pMotionSet);
   g_pMotionSet->Clear();
   g_pMotionDatabase->BuildFromAscii();

   if (!!pAIManager)
      pAIManager->ResetMotionTags();
}

static void SaveMotionDatabase()
{
   ITagFile *pFile=TagFileOpen("motiondb.bin",kTagOpenWrite);

   ITagFile_OpenBlock(pFile,&g_MotDBTag,&g_MotDBVersion);
   g_pMotionSet->Save(pFile);
   g_pMotionDatabase->Save(pFile);
   ITagFile_CloseBlock(pFile);

   SafeRelease(pFile);
}

Command MotMngrCommands[] =
{
   { "build_motion_database", FUNC_VOID, BuildMotionDatabase, "read in motion schemas as build database" },
   { "save_motion_database", FUNC_VOID, SaveMotionDatabase, "save out current built motion database" },
};
#endif

void MotionManagerInit()
{
   g_pMotionSet=NewMotionSet();
   g_pMotionDatabase = new cMotionDatabase;
#ifdef EDITOR
   COMMANDS(MotMngrCommands,HK_EDITOR);
#endif
   CoreMotionPropsInit();
}

void MotionManagerClose()
{
   delete g_pMotionSet;
   g_pMotionSet=NULL;

   delete g_pMotionDatabase;
   g_pMotionDatabase=NULL;

   CoreMotionPropsShutdown();
}

void MotionManagerReset()
{
   g_pMotionSet->Clear();
   g_pMotionDatabase->Clear();
}

void MotionManagerLoad()
{
   AUTO_TIMER(MOTMNGR_PF_LOAD);
   AutoAppIPtr(AIManager);

   // first check current directory
   ITagFile *pFile=TagFileOpen("motiondb.bin",kTagOpenRead);

   if(!pFile) // try resname path
   {
      char fullname[80];

      if(!find_file_in_config_path(fullname,"mschema/motiondb.bin","resname_base"))
      {
         Warning(("Unable to load motion database\n"));
         return;
      }
      pFile=TagFileOpen(fullname,kTagOpenRead);
      if(!pFile)
      {
         Warning(("Unable to load motion database\n"));
         return;
      }
   }

   TagVersion FoundVersion = g_MotDBVersion;
   HRESULT result = ITagFile_OpenBlock(pFile,&g_MotDBTag,&FoundVersion);

   if (result == S_OK
    && FoundVersion.major == g_MotDBVersion.major
    && FoundVersion.minor == g_MotDBVersion.minor) {
      g_pMotionSet->Load(pFile);
      g_pMotionDatabase->Load(pFile);
      ITagFile_CloseBlock(pFile);
      SafeRelease(pFile);
   }

   if (!!pAIManager)
      pAIManager->ResetMotionTags();
}
