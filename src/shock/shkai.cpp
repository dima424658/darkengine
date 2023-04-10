/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkai.cpp,v 1.21 1999/11/19 14:56:39 adurant Exp $

#include <shkai.h>

#include <appagg.h>

#include <shkaiabs.h>
#include <shkaicbs.h>
#include <shkaidmr.h>
#include <shkaigrb.h>
#include <shkaimbs.h>
#include <shkaipr.h>
#include <shkaiprb.h>
#include <shkaipro.h>
#include <shkaipth.h>
#include <shkairbs.h>
#include <shkaireb.h>
#include <shkairnb.h>
#include <shkaircp.h>
#include <shkaishb.h>
#include <shkaisho.h>
#include <shkaiswm.h>
#include <aiturbs.h>
 
#include <aiman.h>
#include <relation.h>
#include <linkbase.h>

// AI vision
#include <aiprcore.h>
#include <aivision.h>
#include <sdesbase.h>
#include <sdesc.h>

// commands
#include <ai.h>
#include <aiprrngd.h>
#include <command.h>
#include <iobjsys.h>
#include <objquery.h>
#include <traitbas.h>
#include <traitman.h>

// must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////
// struct desc for vision types

// note: must keep this in synch with eAIVisionType
static char *visionTypes[] = 
{
   "Generic", 
   "Camera",
   "Robot", 
   "Human",
   "Hybrid",
   "Annelid",
};

static sFieldDesc _g_AIVisionTypeFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIVisionTypeNum, kAIVisionTypeNum, visionTypes},
}; 

static sStructDesc _g_AIVisionTypeStructDesc = 
{
   PROP_AI_VISTYPE,
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AIVisionTypeFieldDesc)/sizeof(_g_AIVisionTypeFieldDesc[0]),
   _g_AIVisionTypeFieldDesc,
}; 

////////////////////////////////////////////////////////
// 
// Commands
//

//
// Iterate over all AIs, destroy their existing projectiles and re-create them
//

#ifndef SHIP
void ReinstantiateAIProjectiles(void)
{
   AutoAppIPtr(AIManager);
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);
   IAI* pAI;
   tAIIter iter;
   IObjectQuery* pTraitQuery;
   BOOL found;

   pAI = pAIManager->GetFirst(&iter);
   while (pAI != NULL)
   {
      pTraitQuery = pTraitManager->Query(pAI->GetObjID(), kTraitQueryAllArchetypes);
      found = FALSE;
      while (!pTraitQuery->Done() && !found)
      {
         if (g_pRangedCombatProp->IsRelevant(pTraitQuery->Object()))
            found = TRUE;
         else
            pTraitQuery->Next();
      }
      if (found)
      {
         // reinstantiate them
         g_pRangedCombatProp->Delete(pAI->GetObjID());
         g_pRangedCombatProp->Create(pAI->GetObjID());
      }
      SafeRelease(pAI);
      pAI = pAIManager->GetNext(&iter);
   }
   pAIManager->GetDone(&iter);
}

static Command aiCommands[] =
{
   { "fix_ai_proj", FUNC_VOID, ReinstantiateAIProjectiles, "Reinstantiate AI projectiles & links"},
};

#endif

////////////////////////////////////////////////////////

void ShockAIInit(void)
{
   AutoAppIPtr(AIManager);

   // Properties
   ShockAIInitSwarmProp();
   ShockAIInitGrubCombatProp();
   ShockAIInitWanderProp();
   ShockAIInitRangedProp();
   ShockAIInitMeleeTargetProp();

   ShockAIInitProtocolDroid();
   ShockAIInitShodan();

   ShockAIInitDoorMovReg();

   // Create behavior sets
   // Shock default
   IAIBehaviorSet * pBehaviorSet = new cAIShockBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // turret
   pBehaviorSet = new cAITurretBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // camera
   pBehaviorSet = new cAICameraBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // swarmer
   pBehaviorSet = new cAISwarmerBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // grub
   pBehaviorSet = new cAIGrubBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // protocol
   pBehaviorSet = new cAIProtocolBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // shodan
   pBehaviorSet = new cAIShodanBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // default ranged
   pBehaviorSet = new cAIShockRangedBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // ranged explode
   pBehaviorSet = new cAIShockRangedExplodeBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // default melee
   pBehaviorSet = new cAIMeleeBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // default ranged/melee
   pBehaviorSet = new cAIShockRangedMeleeBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // cut-scene actor
   pBehaviorSet = new cAIActorBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   // struct descs
   StructDescRegister(&_g_AIVisionTypeStructDesc);

#ifndef SHIP
   // commands
   COMMANDS(aiCommands,HK_ALL);
#endif
}

void ShockAITerm(void)
{
   ShockAITermShodan();
   ShockAITermSwarmProp();
   ShockAITermWanderProp();
   ShockAITermGrubCombatProp();
   ShockAITermRangedProp();
   ShockAITermDoorMovReg();
   ShockAITermPathfinder();
}
