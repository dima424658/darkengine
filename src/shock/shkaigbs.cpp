/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigbs.cpp,v 1.4 1999/03/25 17:35:20 JON Exp $
//
//
//

#include <shkaigbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aibassns.h>
#include <aibassns.h>
#include <aicbrngd.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aimove.h>
#include <ainet.h>
#include <aipthfnd.h>
#include <aisound.h>
#include <aistdmrg.h>

#include <shkaidmr.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGruntBehaviorSet
//

STDMETHODIMP_(const char *) cAIGruntBehaviorSet::GetName()
{
   return "Grunt";
}

///////////////////////////////////////

void cAIGruntBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIFreeze);
   cAIBehaviorSet::CreateNonCombatAbilities(pComponents);
}

///////////////////////////////////////

void cAIGruntBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICombatRanged);
}

////////////////////////////////////
// 
// This is standard, but I took out
//
// door movement regulator
// defend 
//
// and added
// shock door movement regulator
//

void cAIGruntBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
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
   pComponents->Append(new cAIPathfinder);
   
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

