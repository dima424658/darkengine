/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiswm.cpp,v 1.4 1999/11/19 14:57:00 adurant Exp $
//
//
//

#include <lg.h>

#include <comtools.h>
#include <appagg.h>
#include <property.h>
#include <propman.h>
#include <propface.h>

#include <ai.h>
#include <aiutils.h>

#include <aiqdeath.h>
#include <shkaiswa.h>
#include <shkaiswm.h>
#include <shkaiwnd.h>

#include <linkman.h>
#include <relation.h>
#include <autolink.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISwarmer
//

STDMETHODIMP_(float) cAISwarmer::GetGroundOffset()
{
   return cAI::GetGroundOffset() + sin(((AIGetTime() % 4000) / 2000.0) * PI) * 0.5;
}

///////////////////////////////////////////////////////////////////////////////
 
void cAISwarmerBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIWander *pWanderAbility = new cAIWander;
   pComponents->Append(pWanderAbility);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAISwarmerBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAISwarm *pSwarmAbility = new cAISwarm;
   pComponents->Append(pSwarmAbility);
}

///////////////////////////////////////////////////////////////////////////////
 
void cAISwarmerBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
}

