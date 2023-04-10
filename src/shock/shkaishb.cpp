/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaishb.cpp,v 1.2 1999/03/31 18:51:43 JON Exp $
//
//
//

#include <shkaishb.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>
#include <property.h>
#include <propman.h>
#include <propface.h>

#include <ai.h>
#include <aifreeze.h>
#include <aiutils.h>

#include <shkaisho.h>

#include <linkman.h>
#include <relation.h>
#include <autolink.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////

void cAIShodanBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAIShodanCombat *pShodanCombatAbility = new cAIShodanCombat;
   pComponents->Append(pShodanCombatAbility);
}
