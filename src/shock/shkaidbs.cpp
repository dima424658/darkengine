/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaidbs.cpp,v 1.3 1999/03/31 18:48:31 JON Exp $
//
//
//

#include <shkaidbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicbrngd.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aisound.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDroidBehaviorSet
//

STDMETHODIMP_(const char *) cAIDroidBehaviorSet::GetName()
{
   return "Droid";
}

///////////////////////////////////////

void cAIDroidBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICombatRanged);
}

