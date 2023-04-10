/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmiss.cpp,v 1.5 2000/02/19 12:27:22 toml Exp $
#include <drkmiss.h>
#include <filevar.h>
#include <appagg.h>
#include <sdestool.h>
#include <sdesbase.h>
#include <dbasemsg.h>

#include <tagpatch.h>
#include <dbfile.h>
#include <dbtagfil.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// SDESC
//

static sFieldDesc mission_fields[] = 
{
   { "Mission Number",  kFieldTypeInt, FieldLocation(sMissionData,num) }, 
   { "Mission Pathname",  kFieldTypeString, FieldLocation(sMissionData,path) }, 

};


static sStructDesc mission_sdesc = StructDescBuild(sMissionData,kStructFlagNone,mission_fields); 

//
// VAR DESCRIPTOR
//


sFileVarDesc gMissionVarDesc = 
{
   kMissionVar,         // Where do I get saved?
   "DARKMISS",          // Tag file tag
   "Dark Mission Description",     // friendly name
   FILEVAR_TYPE(sMissionData),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "dark"
}; 


//
// Actually use the defaults when resetting
//

class cMissionData : public cFileVar<sMissionData,&gMissionVarDesc>
{

};

//
// The variable itself 
//
static cMissionData gMissionData; 

const sMissionData* GetMissionData()
{
   return &gMissionData; 
}

void LoadMissionData(ITagFile* file)
{
   gMissionData.DatabaseMsg(kDatabaseLoad|kDBMission,file); 
}

void SaveMissionData(ITagFile* file)
{
   gMissionData.DatabaseMsg(kDatabaseSave|kDBMission,file); 
}


void MissionDataInit()
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&mission_sdesc); 
}

void MissionDataTerm()
{
}

static ITagFile* find_and_open_file(const char* filename)
{
   char full[256]; 
   if (dbFind(filename,full))
      return dbOpenFile(full,kTagOpenRead);
   return NULL; 
   
}

ITagFile* OpenMissionFile(int missnum)
{
   // construct the base filename
   char base[16]; 
   sprintf(base,"miss%d.ext",missnum); 

   // find the extension 
   char* ext = strchr(base,'.') + 1; 
   
   // Open the mis file
   strcpy(ext,"mis");  
   ITagFile* basefile = find_and_open_file(base); 

   if (!basefile)
      return NULL; 

   // Open the dif file
   strcpy(ext,"dif"); 
   ITagFile* diffile = find_and_open_file(base); 

   if (diffile)
   {
      ITagFile* result = CreatePatchedTagFile(basefile,diffile); 
      SafeRelease(basefile); 
      SafeRelease(diffile);
      return result;
   }
   else 
      return basefile; 
}


