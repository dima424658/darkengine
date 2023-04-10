/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mskilset.cpp,v 1.23 1998/10/13 13:10:40 KATE Exp $

#include <mskilset.h>
#include <motdesc.h>
#include <fix.h>
#include <motdb.h>
#include <motschem.h>
#include <motmngr.h>
#include <mschutil.h>
#include <mclntapi.h>
#include <config.h>
#include <cfgdbg.h>
#include <mmanuver.h>
#include <motprop.h>

#include <dbmem.h>



//////////////////////////////////////////////
//
// Motion SkillSet Stuff
//
//////////////////////////////////////////////

void cMSkillSet::SetActorType(int type)
{
   m_ActorType=type;
}

// @TODO: Get rid of this function everywhere
void cMSkillSet::SetPersistentTags(const cTagSet *pTags)
{
   return;
}

static cTagDBInput g_Tags;

static void SetupTags(const IMotor *pMotor,const cTagSet *pTags,sMotActorTagList **ppActTags)
{
   sMotActorTagList *pActorTags=NULL;

   // add on persistent (actor) tags that always get associated with
   // motion request
   g_Tags.Clear();
   g_Tags.AppendTagSet(pTags); // cast away constness

   // get persistent tags
   if(pMotor && ObjGetActorTagList(pMotor->GetObjID(),&pActorTags))
   {
      cTagSet actorTagSet;

      actorTagSet.FromString(pActorTags->m_TagStrings);
      g_Tags.AppendTagSet(&actorTagSet);
      *ppActTags=pActorTags;
   } else
   {
      *ppActTags=NULL;
   }
}

cMotionPlan *cMSkillSet::BuildPlanFromParams(const sMcMoveParams *pParams,\
      const sMcMotorState *pMotorState, const sMcMoveState *pMoveState, \
      IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cMotionSchema *pSchema;
   sMotActorTagList *pActorTags=NULL;
   cStr spewString;  
   BOOL doSpew=FALSE;

#ifndef SHIP
   doSpew=config_is_defined("MCoordTrace");
#endif

   // NOTE: if specifying motion by name or number, still need schema
   // for archetype

   Assert_(pParams);

   SetupTags(pMotor,&pParams->tags,&pActorTags);

   if(doSpew)
   {
      pParams->tags.ToString(&spewString);
      if(pActorTags)
      {
         spewString.Append(2,", ");
         spewString.Append(strlen(pActorTags->m_TagStrings),pActorTags->m_TagStrings);
      }
      spewString.Append(4,"} : ");
   }

   if(!g_pMotionDatabase->GetBestMatch(m_ActorType,&g_Tags,&pSchema))
   {
      if(doSpew)
      {
         spewString.Append(8,"no match");
         mprintf("tag request {%s\n",spewString);
      }
      return NULL;
   }

   Assert_(pSchema);

   if(doSpew)
   {
      char buf[32];

      sprintf(buf,"match %d : %d mots",pSchema->GetSchemaID(),pSchema->NumMotions());
      buf[31]='\0';
      spewString.Append(strlen(buf),buf);
      mprintf("tag request {%s\n",spewString);
   }

   int id;

   if(!MSchUGetMotControllerID(pSchema,&id))
   {
      Warning(("No motor controller for schema\n"));
      return NULL;
   }
   cMotionPlan *pPlan=g_ManeuverFactoryList[id]->CreatePlan(pSchema,*pMotorState,*pMoveState,*pParams,pMotor,pCoord);
   // set app data to controller id.  secretly know only one maneuver in plan - KJ
   if(pPlan)
   {
      IManeuver *pMnvr=pPlan->GetFirstManeuver();
      pMnvr->SetAppData(id); // store this for load/save purposes KJ 10/98
   }
   return pPlan;
}


void cMSkillSet::Pose(const cTagSet *pTags, float frac,IMotor *pMotor)
{
   sMotActorTagList *pActTagNames;
   cMotionSchema *pSchema;
   int motNum;

   SetupTags(pMotor,pTags,&pActTagNames);

   if(g_pMotionDatabase->GetBestMatch(m_ActorType,&g_Tags,&pSchema))
   {
      if(pSchema->GetMotion(0,&motNum))
      {
         pMotor->PoseAtMotionFrame(motNum,frac*(g_pMotionSet->GetNumFrames(motNum)-1));
      }
   }
}
