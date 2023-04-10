/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprabil.h,v 1.6 1999/02/10 09:45:58 JON Exp $
//
// @TBD (toml 06-09-98): these should be moved into their corresponding
// ability files?
//
//

#ifndef __AIPRABL_H
#define __AIPRABL_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

typedef unsigned eAIInvestKind;
typedef unsigned eAINonHostility;

///////////////////////////////////////////////////////////////////////////////

void AIInitAbilProps();
void AITermAbilProps();

///////////////////////////////////////////////////////////////////////////////
//
// ABILITY SETTINGS
//

///////////////////////////////////////
// 
// Patrolling
//

#define PROP_AI_PATROL        "AI_Patrol"
#define PROP_AI_PATROL_RANDOM "AI_PtrlRnd"

EXTERN IBoolProperty * g_pAIPatrolProperty;
EXTERN IBoolProperty * g_pAIPatrolRandomProperty;

#define AIShouldPatrol(obj) AIGetProperty(g_pAIPatrolProperty, (obj), (BOOL)(FALSE))
#define AIPatrolsRandom(obj) AIGetProperty(g_pAIPatrolRandomProperty, (obj), (BOOL)(FALSE))

// Property observer, reset each frame
class cPropertyObserver;
extern cPropertyObserver * g_pAIPatrolPropObs;

///////////////////////////////////////
// 
// Idling
//

#define PROP_AI_FIDGET "AI_Fidget"

EXTERN IBoolProperty * g_pAIFidgetProperty;

#define AIShouldIdleFidget(obj) AIGetProperty(g_pAIFidgetProperty, (obj), (BOOL)(TRUE))

///////////////////////////////////////
//
// Investigation property
//

#undef INTERFACE
#define INTERFACE IAIInvestKindProperty
DECLARE_PROPERTY_INTERFACE(IAIInvestKindProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(eAIInvestKind);
};

///////////////////

#define PROP_AI_INVESTKIND  "AI_InvKnd"

EXTERN IAIInvestKindProperty * g_pAIInvestKindProperty;

#define AIGetInvestKind(obj) AIGetProperty(g_pAIInvestKindProperty, (obj), ((eAIInvestKind)0))

///////////////////////////////////////
//
// Non-hostility property
//

#undef INTERFACE
#define INTERFACE IAINonHostilityProperty
DECLARE_PROPERTY_INTERFACE(IAINonHostilityProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(eAINonHostility);
};

///////////////////

#define PROP_AI_NONHOSTILITY  "AI_NonHst"

EXTERN IAINonHostilityProperty * g_pAINonHostilityProperty;

#define AIGetNonHostility(obj) AIGetProperty(g_pAINonHostilityProperty, (obj), ((eAINonHostility)0))

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_NCDmRsp2", Complex
//

struct sAINonCombatDmgResp
{
   int woundThreshold;
   int severeThreshold;
   int responseChance;
};
 
////////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IAINonCombatDmgRespProperty
DECLARE_PROPERTY_INTERFACE(IAINonCombatDmgRespProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAINonCombatDmgResp *);
};

#define PROP_AI_NONCOMBATDMGRESP "AI_NCDmgRsp"

EXTERN IAINonCombatDmgRespProperty* g_pAINonCombatDmgRespProperty;

#pragma pack()

#endif /* !__AIPRABL_H */
