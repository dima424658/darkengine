/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicpinit.cpp,v 1.32 2000/03/25 22:10:01 adurant Exp $
//
//
//

#include <aicpinit.h>

#include <aiactmrk.h>
#include <aialtrsp.h>
#include <aiapi.h>
#include <aibassns.h>
#include <aicamera.h>
#include <aicbhtoh.h>
#include <aicbrnew.h>
#include <aicbrngd.h>
#include <aicbtrsp.h>
#include <aicdummy.h>
#include <aicnvabl.h>
#include <aicombat.h>
#include <aidefend.h>
#include <aiflee.h>
#include <aifollow.h>
#include <aifreeze.h>
#include <aiidle.h>
#include <aiinform.h>
#include <ailisten.h>
#include <aipatrol.h>
#include <aiscrabl.h>
#include <aisound.h>
#include <aithreat.h>
#include <aiwatch.h>
#include <aiavoid.h>
#include <aipthobb.h>
#include <aipthfnd.h>
#include <aidoormr.h>
#include <aincbdmg.h>
#include <aistun.h>
#include <aisusrsp.h>
#include <aiinvest.h>
#include <aibdyrsp.h>
#include <aimultcb.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitStandardComponents(IAIManager * pManager)
{
   AIInitPatrolAbility(pManager);
   AIInitCombatAbility(pManager);
   AIInitRangedCombatAbility(pManager);
   AIInitNewRangedCombatAbility(pManager);
   AIInitCombatDummyAbility(pManager);
   AIInitCombatHtoHAbility(pManager);
   AIInitScriptAbility(pManager);
   AIInitFleeAbility(pManager);
   AIInitWatchAbility(pManager);
   AIInitDefendAbility(pManager);
   AIInitIdleAbility(pManager);
   AIInitCameraAbility(pManager);
   AIInitMarkerAction();
   AIInitSenses();
   AIInitSoundEnactor(pManager);
   AIInitFollowAbility(pManager);
   AIInitSignalResponseAbility(pManager);
   AIInitThreatResponseAbility(pManager);
   AIInitAlertResponseAbility(pManager);
   AIInitBodyResponseAbility(pManager);
   AIInitSuspiciousResponseAbility(pManager);
   AIInitObjectAvoid(pManager);
   AIInitConverseAbility(pManager);
   AIInitObjectPathable(pManager);
   AIInitInformAbility(pManager);
   AIInitSenseCombatResponseAbility(pManager);
   AIInitFreezeAbility(pManager);
   AIInitPathfinder(pManager);
   AIInitDoorMovReg(pManager);
   AIInitNonCombatDamageResponseAbility(pManager);
   AIInitStunAbility();
   AIInitInvestigateAbility();
   AIInitMultiCombat(pManager);
   return TRUE;
}

///////////////////////////////////////

BOOL AITermStandardComponents()
{
   AITermScriptAbility();
   AITermCameraAbility();
   AITermCombatHtoHAbility();
   AITermCombatDummyAbility();
   AITermCombatAbility();
   AITermRangedCombatAbility();
   AITermPatrolAbility();
   AITermFleeAbility();
   AITermWatchAbility();
   AITermDefendAbility();
   AITermIdleAbility();
   AITermMarkerAction();
   AITermSenses();
   AITermSoundEnactor();
   AITermFollowAbility();
   AITermSignalResponseAbility();
   AITermThreatResponseAbility();
   AITermAlertResponseAbility();
   AITermBodyResponseAbility();
   AITermSuspiciousResponseAbility();
   AITermObjectAvoid();
   AITermObjectPathable();
   AITermInformAbility();
   AITermSenseCombatResponseAbility();
   AITermFreezeAbility();
   AITermPathfinder();
   AITermDoorMovReg();
   AITermNonCombatDamageResponseAbility();
   AITermStunAbility();
   AITermInvestigateAbility();
   AITermMultiCombat();
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIDBResetStandardComponents()
{
   AIDBResetObjectAvoid();
   return TRUE;
}
