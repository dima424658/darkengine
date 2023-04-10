/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#include <dpcairbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aiutils.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aisound.h>

#include <dpcaimuc.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCRangedMeleeBehaviorSet
//

STDMETHODIMP_(const char *) cAIDPCRangedMeleeBehaviorSet::GetName()
{
   return "RangedMelee";
}

///////////////////////////////////////

void cAIDPCRangedMeleeBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIDPCMultiCombat);
}

