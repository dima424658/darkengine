/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcai.h>

#include <appagg.h>

#include <dpcaiabs.h>
#include <dpcaicbs.h>
#include <dpcaidgb.h>  // Dog behavior set.
#include <dpcaidmr.h>
#include <dpcaimbs.h>
#include <dpcaipr.h>
#include <dpcaipro.h>
#include <dpcaipth.h>
#include <dpcairbs.h>
#include <dpcaireb.h>
#include <dpcairnb.h>
#include <dpcaircp.h>
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
   "Dog",
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
   tAIIter iter;
   IObjectQuery* pTraitQuery;
   BOOL found;

   IAI* pAI = pAIManager->GetFirst(&iter);
   while (pAI != NULL)
   {
      pTraitQuery = pTraitManager->Query(pAI->GetObjID(), kTraitQueryAllArchetypes);
      found = FALSE;
      while (!pTraitQuery->Done() && !found)
      {
         if (g_pRangedCombatProp->IsRelevant(pTraitQuery->Object()))
         {
             found = TRUE;
         }
         else
         {
             pTraitQuery->Next();
         }
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

void DPCAIInit(void)
{
   AutoAppIPtr(AIManager);

   // Properties
   DPCAIInitWanderProp();
   DPCAIInitRangedProp();
   DPCAIInitMeleeTargetProp();
   DPCAIInitDogCombatProp();

   DPCAIInitDoorMovReg();

   // Create behavior sets
   // Deep Cover default
   IAIBehaviorSet * pBehaviorSet = new cAIDPCBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // Dog
   pBehaviorSet = new cAIDogBehaviorSet;
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
   // default ranged
   pBehaviorSet = new cAIDPCRangedBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // ranged explode
   pBehaviorSet = new cAIDPCRangedExplodeBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // default melee
   pBehaviorSet = new cAIMeleeBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
   // default ranged/melee
   pBehaviorSet = new cAIDPCRangedMeleeBehaviorSet;
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

void DPCAITerm(void)
{
   DPCAITermWanderProp();
   DPCAITermDogCombatProp();
   DPCAITermRangedProp();
   DPCAITermDoorMovReg();
   DPCAITermPathfinder();
}
