/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkairbs.cpp,v 1.2 1999/03/31 18:51:14 JON Exp $
//
//
//

#include <shkairbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aiutils.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aisound.h>

#include <shkaimuc.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedMeleeBehaviorSet
//

STDMETHODIMP_(const char *) cAIShockRangedMeleeBehaviorSet::GetName()
{
   return "RangedMelee";
}

///////////////////////////////////////

void cAIShockRangedMeleeBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIShockMultiCombat);
}

