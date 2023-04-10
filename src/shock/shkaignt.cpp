/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaignt.cpp,v 1.1 1998/06/22 11:32:53 JON Exp $
//
//
//
#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicbrngd.h>
#include <shkaignt.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGruntBehaviorSet
//

STDMETHODIMP_(const char *) cAIGruntBehaviorSet::GetName()
{
   return "Grunt";
}

///////////////////////////////////////

void cAIGruntBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICombatRanged);
}

