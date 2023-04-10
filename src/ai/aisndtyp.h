/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisndtyp.h,v 1.14 2000/03/06 22:36:28 bfarquha Exp $
//
//
#pragma once

#ifndef __AISNDTYP_H
#define __AISNDTYP_H

///////////////////////////////////////////////////////////////////////////////

enum eAISoundType
{
   kAIST_None,
   kAIST_Inform,
   kAIST_MinorAnomaly,
   kAIST_MajorAnomaly,
   kAIST_NonCombatAlert,
   kAIST_Combat,

   kAIST_Num,

   kAIST_TypeMax=0xffffffff // force it use an int
};

struct sAISoundType
{
   eAISoundType type;
   char         szSignal[32];
   int          range;
};

///////////////////////////////////////

struct sAISoundTweaks
{
   int defaultRanges[kAIST_Num];
};

extern sAISoundTweaks * g_pAISoundTweaks;

///////////////////////////////////////////////////////////////////////////////

typedef unsigned eAISoundConcept;

enum eAISoundConceptEnum
{
   kAISC_CoreBase = 0,

   // Non-awareness broadcast
   kAISC_Sleeping,
   kAISC_AlertZeroBroadcast,
   kAISC_AlertOneBroadcast,
   kAISC_AlertTwoBroadcast,
   kAISC_AlertThreeBroadcast,

   // Transitions
   kAISC_AlertToOne,
   kAISC_AlertToTwo,
   kAISC_AlertToThree,
   kAISC_SpotPlayer,

   kAISC_AlertDownToZero,
   kAISC_LostContact,

   // Reactions
   kAISC_ReactCharge,
   kAISC_ReactShoot,
   kAISC_ReactRun,
   kAISC_ReactFriend,
   kAISC_ReactAlarm,
   kAISC_ReactFindFriend,

   // Combat related
   kAISC_CombatAttack,
   kAISC_CombatSuccHit,
   kAISC_CombatSuccBlock,
   kAISC_CombatDetBlock,
   kAISC_CombatBlocked,
   kAISC_CombatHitNoDam,
   kAISC_CombatHitDamageHigh,
   kAISC_CombatHitDamageLow,
   kAISC_CombatHitAmbush,

   // Death
   kAISC_DieLoud,
   kAISC_DieSoft,

   // Discoveries
   kAISC_FoundBody,
   kAISC_FoundMissing,
   kAISC_FoundBreach,
   kAISC_FoundSmall,
   kAISC_FoundLarge,
   kAISC_FoundRobot,
   kAISC_RecentIntruder,
   kAISC_RecentBody,
   kAISC_RecentMissing,
   kAISC_RecentOther,
   kAISC_RecentRobot,

   kAISC_OutOfReach,

   kAISC_CoreNum,

   ////////////////

   kAISC_GameBase = 10000,

   ////////////////

   kAISC_TypeMax=0xffffffff // force it use an int
};

///////////////////////////////////////

EXTERN const char * AIGetConceptName(eAISoundConcept);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AISNDTYP_H */
