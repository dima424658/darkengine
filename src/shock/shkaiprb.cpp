/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiprb.cpp,v 1.5 1999/11/19 14:56:52 adurant Exp $
//
//
//

#include <shkaiprb.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>
#include <property.h>
#include <propman.h>
#include <propface.h>

#include <ai.h>
#include <aidetect.h>
#include <aifreeze.h>
#include <aiscrabl.h>
#include <aistun.h>
#include <aiutils.h>

#include <shkaipro.h>

#include <linkman.h>
#include <relation.h>
#include <autolink.h>

#include <aiqdeath.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////

void cAIProtocolBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIProtocolCombat *pProtocolCombatAbility = new cAIProtocolCombat;
   pComponents->Append(pProtocolCombatAbility);
}

////////////////////////////////////

void cAIProtocolBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
   pComponents->Append(new cAIBasicScript);
   pComponents->Append(new cAIStun);
   pComponents->Append(new cAIDetect);
}
