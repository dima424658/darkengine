/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#include <dpcaiabs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aibassns.h>
#include <aibassns.h>
#include <aicnvabl.h>
#include <aidbgcmp.h>
#include <aideath.h>
#include <aiidle.h>
#include <ailisten.h>
#include <aimove.h>
#include <ainet.h>
#include <aipatrol.h>
#include <aipthfnd.h>
#include <aiscrabl.h>
#include <aisound.h>
#include <aistdmrg.h>
#include <aiwatch.h>

#include <dpcaidmr.h>
#include <dpcaipth.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIActorBehaviorSet
//

STDMETHODIMP_(const char *) cAIActorBehaviorSet::GetName()
{
   return "DPCActor";
}

///////////////////////////////////////

void cAIActorBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIDeath);
   pComponents->Append(new cAIBasicScript);
}

////////////////////////////////////

void cAIActorBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIPatrol);
   pComponents->Append(new cAIIdle);
   pComponents->Append(new cAISignalResponse);
   pComponents->Append(new cAIWatch);
   pComponents->Append(new cAIConverse);
}

///////////////////////////////////////

void cAIActorBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
}
