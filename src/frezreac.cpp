/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/frezreac.cpp,v 1.6 2000/01/24 20:13:46 adurant Exp $
//

#include <frezreac.h>

#include <appagg.h>

#include <reaction.h>
#include <reacbase.h>
#include <stimbase.h>

#include <aiapi.h>

#include <sdestool.h>
#include <sdesbase.h>

// must be last header
#include <dbmem.h>


//
// Freeze AI reaction
//

//
// The parameters
//

struct sFreezeReactionParam
{
   tSimTime duration;
};

//
// The reaction
// Stim intensity is a multiplier on duration 
//

static eReactionResult LGAPI freezeReactionResult(sReactionEvent* pEvent, const sReactionParam* param, tReactionFuncData )
{
   sFreezeReactionParam* pParam = (sFreezeReactionParam*)&param->data; 
   IAI* pAI;
   AutoAppIPtr(AIManager);

   if ((pAI = pAIManager->GetAI(pEvent->sensor_obj)) == NULL)
      return kReactionAbort;
//making time in milliseconds so that we can have finer control over freeze
//times wrt strength of stim. AMSD
   pAI->Freeze(pParam->duration*pEvent->stim->intensity);
   pAI->Release();
   return kReactionNormal; 
}

static sReactionDesc freezeReactionDesc = 
{
   "Freeze",
   "Freeze AI", 
   REACTION_PARAM_TYPE(sFreezeReactionParam),
}; 


//
// The sdesc
//

static sFieldDesc freezeReactionFDesc[] = 
{
   { "Duration Multiplier", kFieldTypeInt, FieldLocation(sFreezeReactionParam,duration) }
};

static sStructDesc freezeReactionSDesc = StructDescBuild(sFreezeReactionParam,kStructFlagNone,freezeReactionFDesc); 

//
// Init/Term 
//

EXTERN void FreezeReactionsInit()
{
   AutoAppIPtr(Reactions); 
   pReactions->Add(&freezeReactionDesc,freezeReactionResult,NULL); 
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&freezeReactionSDesc); 
}

EXTERN void FreezeReactionsTerm()
{
}
