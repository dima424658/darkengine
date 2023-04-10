/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#include <dpcaimbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aibassns.h>
#include <aibassns.h>
#include <aicbhtoh.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aimove.h>
#include <ainet.h>
#include <aipthfnd.h>
#include <aisound.h>
#include <aistdmrg.h>

#include <dpcaidmr.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMeleeBehaviorSet
//

STDMETHODIMP_(const char *) cAIMeleeBehaviorSet::GetName()
{
   return "Melee";
}

///////////////////////////////////////

void cAIMeleeBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICombatHtoH);
}

