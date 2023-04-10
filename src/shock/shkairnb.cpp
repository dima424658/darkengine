/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkairnb.cpp,v 1.2 1999/04/11 11:06:50 JON Exp $
//
//
//

#include <shkairnb.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicbrngd.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aisound.h>
#include <aiutils.h>

#include <shkaicbr.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedBehaviorSet
//

STDMETHODIMP_(const char *) cAIShockRangedBehaviorSet::GetName()
{
   return "Ranged";
} 

///////////////////////////////////////

void cAIShockRangedBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIShockCombatRanged);
}

