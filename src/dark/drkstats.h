/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkstats.h,v 1.5 1999/11/30 14:57:46 adurant Exp $
// dark specific stat gathering

#pragma once

#ifndef __DRKSTATS_H
#define __DRKSTATS_H

////////////////////
// names of the stats we track

#define kDarkStatLoot             "TOTAL_LOOT"  // yea yea, should change this! MONEY_QVAR

#define kDarkStatPickPocket       "DrSPocketOK"
#define kDarkStatPickPocketFail   "DrSPocketFail"
#define kDarkStatPickPocketAble   "DrSPocketCnt"
#define kDarkStatLockPick         "DrSLockPick"
#define kDarkStatLockPickAble     "DrSLkPickCnt"

#define kDarkStatSecrets          "DrSSecrets"
#define kDarkStatTotalSecrets     "DrSScrtCnt"

#define kDarkStatDamageDealt      "DrSDmgDealt"
#define kDarkStatDamageTaken      "DrSDmgTaken"
#define kDarkStatDamageSelf       "DrSDmgSelf"
#define kDarkStatHealingTaken     "DrSHealing"

#define kDarkStatObjDmgDealt      "DrSObjDmg"
#define kDarkStatObjsKilled       "DrSObjKilled"

#define kDarkStatKills            "DrSKills"
#define kDarkStatInnocents        "DrSInnocent"
#define kDarkStatSuicides         "DrSSuicides"

#define kDarkStatBackStabs        "DrSBackStabs"
#define kDarkStatKnockouts        "DrSKnockout"
#define kDarkStatAerials          "DrSAerials"
#define kDarkStatGassed           "DrSGassed"

#define kDarkStatDiscovered       "DrSDiscovery"
#define kDarkStatLootTotal        "DrSLootTotal"
#define kDarkStatBodiesFound      "DrSBodyFound"

#define kDarkStatReloads          "DrSReloads"
#define kDarkStatGameCode         "DrSGameCode"
#define kDarkStatTime             "DrSTime"

#define kDarkStatRobotsKilled     "DrSRobotsKilled"
#define kDarkStatRobotsDeactivated "DrSRobotsDeactivated"

#define kDarkStatCampTime         "DrSCmTime"
#define kDarkStatCampLoot         "DrSCmLoot"
#define kDarkStatCampDamageDealt  "DrSCmDmgDeal"
#define kDarkStatCampDamageTaken  "DrSCmDmgTake"
#define kDarkStatCampKills        "DrSCmKills"
#define kDarkStatCampReloads      "DrSCmReload"


////////////////////
// values of the bits

#define kDarkStatBitInnocent      (1<<0)
#define kDarkStatBitEnemy         (1<<1)
#define kDarkStatBitHidden        (1<<2)
#define kDarkStatBitFoundBody     (1<<3)
#define kDarkStatBitRobot         (1<<4)

EXTERN BOOL DarkStatCheckBit(ObjID obj, int bit);
EXTERN void DarkStatSetBit(ObjID obj, int bit, BOOL val);

////////////////////
// functions to change stats, etc

// for simple integer stats to add a value
EXTERN void DarkStatIntAdd(char *StatName, int val);
// for simple integer stats you want to just increment
#define DarkStatInc(stat) DarkStatIntAdd(stat,1)

// for setting a val directly
EXTERN void DarkStatIntSet(char *StatName, int val);

/////////////////////
// init/term - control

EXTERN void DarkStatInit(void);
EXTERN void DarkStatTerm(void);

EXTERN void DarkStatFinishMission(void);
EXTERN void DarkStatInitMission(void);

#endif  // __DRKSTATS_H
