/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>

#include <comtools.h>
#include <appagg.h>
#include <property.h>
#include <propman.h>
#include <propface.h>

#include <ai.h>
#include <aibassns.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aimove.h>
#include <ainet.h>
#include <aisound.h>
#include <aistdmrg.h>
#include <aiutils.h>
#include <aipatrol.h>

#include <dpcaidmr.h>
#include <aiqdeath.h>
#include <dpcaipth.h>
#include <dpcaiwnd.h>

#include <dpcaidgb.h>
#include <dpcaidga.h>

#include <linkman.h>
#include <relation.h>
#include <autolink.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
 
void cAIDogBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIWander *pWanderAbility   = new cAIWander;
   cAIFreeze *pAIFreezeAbility = new cAIFreeze;
   cAIPatrol *pAIPatrolAbility = new cAIPatrol;
   
   pComponents->Append(pWanderAbility);
   pComponents->Append(pAIFreezeAbility);
   pComponents->Append(pAIPatrolAbility);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAIDogBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIDogCombat *pDogCombatAbility = new cAIDogCombat;
   pComponents->Append(pDogCombatAbility);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAIDogBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAIDogBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
{
   // Debugging/development tools
#ifndef SHIP
   pComponents->Append(new cAIFlowDebugger);
#endif
   
#ifdef TEST_ABILITY
   pComponents->Append(new cAITest);
#endif

   // Enactors
   pComponents->Append(new cAIMoveEnactor);
   pComponents->Append(new cAISoundEnactor);
 
   // Pathfinder 
   pComponents->Append(new cDPCAIPathfinder);
   
   // Movement regulators
   pComponents->Append(new cAIObjectsMovReg);
   pComponents->Append(new cAIWallsCliffsMovReg);
   pComponents->Append(new cDPCAIDoorMovReg);

   // Senses
   pComponents->Append(new cAISenses);
   
#ifdef NEW_NETWORK_ENABLED
   // Networking interface
   pComponents->Append(new cAINetwork);
#endif
}


