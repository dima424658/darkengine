/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/esndreac.cpp,v 1.1 1999/05/29 21:58:28 JON Exp $
//

#include <esndreac.h>

#include <appagg.h>

#include <reaction.h>
#include <reacbase.h>
#include <stimbase.h>

#include <esnd.h>

#include <sdestool.h>
#include <sdesbase.h>

#include <phclsn.h>
#include <phcollev.h>

// must be last header
#include <dbmem.h>


//
// Env Sound reaction
//

//
// The parameters
//

struct sEnvSoundReactionParam
{
   char tags[64];
};

//
// The reaction
// Stim intensity is a multiplier on duration 
//

static eReactionResult LGAPI EnvSoundReactionResult(sReactionEvent* pEvent, const sReactionParam* param, tReactionFuncData )
{
   sEnvSoundReactionParam* pParam = (sEnvSoundReactionParam*)&param->data; 

   cTagSet tagSet(pParam->tags);
   ObjID sourceID = OBJ_NULL;
   sPhysClsnEvent* pClsnEvent = (sPhysClsnEvent*)pEvent->stim->Find(kEventKindCollision);
   if (pClsnEvent != NULL)
      sourceID = pClsnEvent->collision->GetObjID();
   ESndPlay(&tagSet, pEvent->sensor_obj, sourceID);

   return kReactionNormal; 
}

static sReactionDesc EnvSoundReactionDesc = 
{
   "EnvSound",
   "EnvSound", 
   REACTION_PARAM_TYPE(sEnvSoundReactionParam),
}; 


//
// The sdesc
//

static sFieldDesc EnvSoundReactionFDesc[] = 
{
   { "Tags", kFieldTypeString, FieldLocation(sEnvSoundReactionParam, tags) }
};

static sStructDesc EnvSoundReactionSDesc = StructDescBuild(sEnvSoundReactionParam,kStructFlagNone,EnvSoundReactionFDesc); 

//
// Init/Term 
//

EXTERN void EnvSoundReactionInit()
{
   AutoAppIPtr(Reactions); 
   pReactions->Add(&EnvSoundReactionDesc,EnvSoundReactionResult,NULL); 
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&EnvSoundReactionSDesc); 
}

EXTERN void EnvSoundReactionTerm()
{
}
