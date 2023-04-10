/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover AI Ranged Behavior Set

#include <dpcairnb.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicbrngd.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aisound.h>
#include <aiutils.h>

#include <dpcaicbr.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCRangedBehaviorSet
//

STDMETHODIMP_(const char *) cAIDPCRangedBehaviorSet::GetName()
{
   return "Ranged";
} 

///////////////////////////////////////

void cAIDPCRangedBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIDPCCombatRanged);
}

