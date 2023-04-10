/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaireb.cpp,v 1.2 1999/11/19 14:56:58 adurant Exp $
//
//
//

#include <shkaireb.h>

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

#include <shkaicbr.h>
#include <aiqdeath.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedExplodeBehaviorSet
//

STDMETHODIMP_(const char *) cAIShockRangedExplodeBehaviorSet::GetName()
{
   return "RangedExplode";
} 

///////////////////////////////////////

void cAIShockRangedExplodeBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIShockCombatRanged);
}

////////////////////////////////////

void cAIShockRangedExplodeBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
   pComponents->Append(new cAIBasicScript);
   pComponents->Append(new cAIStun);
   pComponents->Append(new cAIDetect);
}


