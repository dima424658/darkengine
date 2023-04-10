/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// Stun AI reaction
//

#include <dpcstnrc.h>

#include <appagg.h>
#include <simtime.h>

#include <reaction.h>
#include <reacbase.h>
#include <stimbase.h>

#include <aiapi.h>
#include <aistnapi.h>

#include <sdestool.h>
#include <sdesbase.h>

// must be last header
#include <dbmem.h>


//
// The parameters
//

struct sStunReactionParam
{
   // how much is enough?
   tSimTime durationMultiplier;
   char tags[100];
};

//
// The reaction
//

static eReactionResult LGAPI StunReactionResult(sReactionEvent* pEvent, const sReactionParam* param, tReactionFuncData )
{
   sStunReactionParam* pParam = (sStunReactionParam*)&param->data; 
   IAI* pAI;
   AutoAppIPtr(AIManager);

   if ((pAI = pAIManager->GetAI(pEvent->sensor_obj)) == NULL)
      return kReactionAbort;
   DoAISetStun(pEvent->sensor_obj, "", pParam->tags, pEvent->stim->intensity*1000*pParam->durationMultiplier);
   pAI->Release();
   return kReactionNormal; 
}

static sReactionDesc StunReactionDesc = 
{
   "Stun",
   "Stun AI", 
   REACTION_PARAM_TYPE(sStunReactionParam),
}; 


//
// The sdesc
//

static sFieldDesc StunReactionFDesc[] = 
{
   { "Duration Multiplier", kFieldTypeInt, FieldLocation(sStunReactionParam,durationMultiplier) },
   { "Tags", kFieldTypeString, FieldLocation(sStunReactionParam,tags) }
};

static sStructDesc StunReactionSDesc = StructDescBuild(sStunReactionParam,kStructFlagNone,StunReactionFDesc); 

//
// Init/Term 
//

EXTERN void StunReactionsInit()
{
   AutoAppIPtr(Reactions); 
   pReactions->Add(&StunReactionDesc,StunReactionResult,NULL); 
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&StunReactionSDesc); 
}

