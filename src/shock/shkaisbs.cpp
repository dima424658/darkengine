/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaisbs.cpp,v 1.3 2000/01/24 20:15:01 adurant Exp $
//
//
//

#include <shkaisbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aibassns.h>
#include <aibassns.h>
#include <aicbrnew.h>
#include <aidbgcmp.h>
//no need to include this now.  AMSD 1/24/00
//#include <aifreeze.h>
#include <aimove.h>
#include <ainet.h>
#include <aipthfnd.h>
#include <aisound.h>
#include <aistdmrg.h>

#include <shkaidmr.h>
#include <shkaipth.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockBehaviorSet
//

STDMETHODIMP_(const char *) cAIShockBehaviorSet::GetName()
{
   return "ShockDefault";
}

///////////////////////////////////////

void cAIShockBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
//Commenting this out because aibasbhv.cpp now appends this.  Double
//appending seems bad.  AMSD 1/24/00
//   pComponents->Append(new cAIFreeze);
   cAIBehaviorSet::CreateNonCombatAbilities(pComponents);
}

///////////////////////////////////////

void cAIShockBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICombatNewRanged);
}

////////////////////////////////////
// 
// This is standard, but I took out
//
// door movement regulator
// defend 
// pathfinder
//
// and added
// shock door movement regulator
// shock pathfinder
//

void cAIShockBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
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

