/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#include <dpcaisbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aibassns.h>
#include <aibassns.h>
#include <aicbrnew.h>
#include <aidbgcmp.h>
//now done in the base class.  So don't need this anymore.
//#include <aifreeze.h>
#include <aimove.h>
#include <ainet.h>
#include <aipthfnd.h>
#include <aisound.h>
#include <aistdmrg.h>

#include <dpcaidmr.h>
#include <dpcaipth.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCBehaviorSet
//

STDMETHODIMP_(const char *) cAIDPCBehaviorSet::GetName()
{
   return "DPCDefault";
}

///////////////////////////////////////

void cAIDPCBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
//now done in the base class.  So don't need this anymore.
//   pComponents->Append(new cAIFreeze);
   cAIBehaviorSet::CreateNonCombatAbilities(pComponents);
}

///////////////////////////////////////

void cAIDPCBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
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
// DPC door movement regulator
// DPC pathfinder
//

void cAIDPCBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
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

