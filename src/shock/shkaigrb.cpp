/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigrb.cpp,v 1.5 1999/11/19 14:56:40 adurant Exp $
//
//
//

#include <shkaigrb.h>

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

#include <shkaidmr.h>
#include <aiqdeath.h>
#include <shkaigra.h>
#include <shkaipth.h>
#include <shkaiwnd.h>

#include <linkman.h>
#include <relation.h>
#include <autolink.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
 
void cAIGrubBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIWander *pWanderAbility = new cAIWander;
   pComponents->Append(pWanderAbility);
   pComponents->Append(new cAIFreeze);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAIGrubBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIGrubCombat *pGrubCombatAbility = new cAIGrubCombat;
   pComponents->Append(pGrubCombatAbility);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAIGrubBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAIGrubBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
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
   pComponents->Append(new cShockAIPathfinder);
   
   // Movement regulators
   pComponents->Append(new cAIObjectsMovReg);
   pComponents->Append(new cAIWallsCliffsMovReg);
   pComponents->Append(new cShockAIDoorMovReg);

   // Senses
   pComponents->Append(new cAISenses);
   
#ifdef NEW_NETWORK_ENABLED
   // Networking interface
   pComponents->Append(new cAINetwork);
#endif
}


