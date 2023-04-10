/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songmiss.cpp,v 1.1 1999/04/26 15:17:30 mwhite Exp $

#include <songmiss.h>
#include <appagg.h>
#include <filevar.h>
//#include <palmgr.h>
#include <resapi.h>
//#include <palrstyp.h>
#include <respaths.h>
//#include <palette.h>

#include <sdestool.h>
#include <sdesbase.h>

#include <config.h>
//#include <csg.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Misc Song Params
//


// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gSongParamsDesc = 
{
   kMissionVar,         // Where do I get saved?
   "SONGPARAMS",        // Tag file tag
   "Song Parameters",   // friendly name
   FILEVAR_TYPE(sMissionSongParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
}; 


// The actual global variable
class cSongParams : public cFileVar<sMissionSongParams,&gSongParamsDesc> 
{
public: 

   // Silly assignment operator 
   cSongParams& operator =(const sMissionSongParams& v) { *(sMissionSongParams*)this = v; return *this; }; 


   void Update()
   {
#ifdef EDITOR
      //      global_ambient = ambient_light*256.0;
#endif
      //pal_set(); 
   }

   void Reset()
   {
      //pal_res[0] = '\0'; 
   }
};

static cSongParams gSongParams;

//------------------------------------------------------------
// SDESC 
//

static sFieldDesc param_field[] = 
{
   { "Song Name", kFieldTypeString, FieldLocation(sMissionSongParams,songName), },
};


static sStructDesc param_sdesc = StructDescBuild(sMissionSongParams,kStructFlagNone,param_field);


//------------------------------------------------------------
// API
// 

void MissionSongInit(void)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&param_sdesc);
}

void MissionSongTerm(void)
{
}


//----------------------------------------


const sMissionSongParams* GetMissionSongParams(void)
{
   return &gSongParams; 
}


void SetMissionSongParams(const sMissionSongParams* params)
{
   gSongParams = *params; 
   gSongParams.Update(); 
}
