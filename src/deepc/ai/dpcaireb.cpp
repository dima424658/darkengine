/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#include <dpcaireb.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicbrngd.h>
#include <aidbgcmp.h>
#include <aidetect.h>
#include <aifreeze.h>
#include <aiscrabl.h>
#include <aisound.h>
#include <aistun.h>
#include <aiutils.h>

#include <dpcaicbr.h>
#include <aiqdeath.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCRangedExplodeBehaviorSet
//

STDMETHODIMP_(const char *) cAIDPCRangedExplodeBehaviorSet::GetName()
{
   return "RangedExplode";
} 

///////////////////////////////////////

void cAIDPCRangedExplodeBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIDPCCombatRanged);
}

////////////////////////////////////

void cAIDPCRangedExplodeBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
   pComponents->Append(new cAIBasicScript);
   pComponents->Append(new cAIStun);
   pComponents->Append(new cAIDetect);
}


