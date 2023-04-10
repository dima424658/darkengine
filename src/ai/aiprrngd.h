/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprrngd.h,v 1.7 2000/01/29 12:45:39 adurant Exp $
//
// Ranged-combat-related properties and relations
//
#pragma once

#ifndef __AIPRRNGD_H
#define __AIPRRNGD_H

#include <property.h>
#include <relation.h>

////////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IVantagePtProperty);
F_DECLARE_INTERFACE(ICoverPtProperty);
F_DECLARE_INTERFACE(IAngleLimitProperty);
F_DECLARE_INTERFACE(IRangedCombatProperty);

////////////////////////////////////////////////////////////////////////////////

enum eAIP_ConstraintType
{
   kCT_None,
   kCT_NumOthers,
   kCT_NumMisses,
};

// Make sure this stays in sync with sAIProjectileRel!
enum eAIP_TargMethod
{
   kTM_StraightLine,
   kTM_Arcing,
   kTM_Reflecting,
   kTM_Overhead,
   kTM_Radius,
};

const kNumTargetingMethods = 5;

////////////////////////////////////////

#define AI_PT_DIBS_NAME "AIPtDibs"

////////////////////////////////////////

#define AI_PROJECTILE_REL_NAME "AIProjectile"

struct sAIProjectileRel
{
   int   constraint_type;
   int   data;

   int   targeting_method;

   int   selection_desire;
   float firing_delay;
   BOOL  leads_target;
   int   ammo;
   int   burst_count;
   int   accuracy;
   int   launch_joint;
   int   launch_vhot;

   float time_since_selected;
   int   num_others_selected;
};

////////////////////////////////////////////////////////////////////////////////

#define AI_VANTAGE_PT_PROP_NAME "AIVantagePt"

struct sVantagePtProp
{
   sVantagePtProp()
   {
      AssertMsg(sizeof(sVantagePtProp()) == 8, "You need to update the constructor for sVantagePtProp");

      value = 0;
      decay_speed = 0.8;
   };

   int   value;
   float decay_speed;
};

#undef  INTERFACE
#define INTERFACE IVantagePtProperty
DECLARE_PROPERTY_INTERFACE(IVantagePtProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sVantagePtProp *);
};

////////////////////////////////////////

#define AI_COVER_PT_PROP_NAME "AICoverPt"

struct sCoverPtProp
{
   sCoverPtProp()
   {
      AssertMsg(sizeof(sCoverPtProp()) == 12, "You need to update the constructor for sCoverPtProp");

      value = 0;
      decay_speed = 0.8;
      can_duck = FALSE;
   };

   int   value;
   float decay_speed;
   BOOL  can_duck;
};

#undef  INTERFACE
#define INTERFACE ICoverPtProperty
DECLARE_PROPERTY_INTERFACE(ICoverPtProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sCoverPtProp *);
};

////////////////////////////////////////

#define ANGLE_LIMIT_PROP_NAME "AngleLimit"

struct sAngleLimitProp
{
   float left_limit;
   float right_limit;
};
 
#undef  INTERFACE
#define INTERFACE IAngleLimitProperty
DECLARE_PROPERTY_INTERFACE(IAngleLimitProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAngleLimitProp *);
};

////////////////////////////////////////////////////////////////////////////////

#define AI_RANGED_COMBAT_PROP_NAME "AIRCProp"

struct sAIRangedCombatProp
{
   int   minimum_distance;
   int   ideal_distance;
   float firing_delay;   
   int   cover_desire;
   float decay_speed;
   int   fire_while_moving;
   BOOL  contain_projectile;
};

#undef  INTERFACE
#define INTERFACE IRangedCombatProperty
DECLARE_PROPERTY_INTERFACE(IRangedCombatProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedCombatProp *);
};

EXTERN sAIRangedCombatProp g_defaultRangedCombatProp;

#define AIGetRangedCombat(obj) AIGetProperty(g_pRangedCombatProp, (obj), &g_defaultRangedCombatProp)

////////////////////////////////////////////////////////////////////////////////

#define AI_RANGED_RANGES "AIRCRanges"

const int kAIRC_NumRangeMarkers = 4;

struct sAIRangedRanges
{
   float m_ranges[kAIRC_NumRangeMarkers];
};

#undef  INTERFACE
#define INTERFACE IAIRangedRangesProperty
DECLARE_PROPERTY_INTERFACE(IAIRangedRangesProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedRanges *);
};

EXTERN sAIRangedRanges g_defaultRangedRanges;

#define AIGetRangedRanges(obj) AIGetProperty(g_pRangedRangesProp, (obj), &g_defaultRangedRanges)

////////////////////////////////////////

#define AI_RANGED_WOUND "AIRCWound"

struct sAIRangedWound
{
   int m_woundThreshold;
   int m_severeThreshold;
   int m_responseChance;
};
 
#undef  INTERFACE
#define INTERFACE IAIRangedWoundProperty
DECLARE_PROPERTY_INTERFACE(IAIRangedWoundProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedWound *);
};

EXTERN sAIRangedWound g_defaultRangedWound;

#define AIGetRangedWound(obj) AIGetProperty(g_pRangedWoundProp, (obj), &g_defaultRangedWound)

////////////////////////////////////////

#define AI_RANGED_WOUND_SOUND "AIRCWndSnd"

#undef  INTERFACE
#define INTERFACE IAIRangedWoundSoundProperty
DECLARE_PROPERTY_INTERFACE(IAIRangedWoundSoundProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedWound *);
};

#define AIGetRangedWoundSound(obj) AIGetProperty(g_pRangedWoundSoundProp, (obj), &g_defaultRangedWound)

////////////////////////////////////////////////////////////////////////////////

EXTERN IRelation             *g_pAIPtDibsRelation;
EXTERN IRelation             *g_pAIProjectileRelation;

EXTERN BOOL AIProjRelListenerLock;

EXTERN IVantagePtProperty    *g_pVantagePtProp;
EXTERN ICoverPtProperty      *g_pCoverPtProp;
EXTERN IAngleLimitProperty   *g_pAngleLimitProp;
EXTERN IRangedCombatProperty *g_pRangedCombatProp;
EXTERN IAIRangedRangesProperty* g_pRangedRangesProp;
EXTERN IAIRangedWoundProperty* g_pRangedWoundProp;
EXTERN IAIRangedWoundSoundProperty* g_pRangedWoundSoundProp;

////////////////////////////////////////////////////////////////////////////////

EXTERN void InitAIProjectileRelation();
EXTERN void TermAIProjectileRelation();

EXTERN void InitAIPtDibsRelation();
EXTERN void TermAIPtDibsRelation();

EXTERN void InitVantagePointProperty();
EXTERN void TermVantagePointProperty();

EXTERN void InitCoverPointProperty();
EXTERN void TermCoverPointProperty();

EXTERN void InitAngleLimitProperty();
EXTERN void TermAngleLimitProperty();

EXTERN void InitRangedCombatProperty();
EXTERN void TermRangedCombatProperty();

EXTERN void InitRangedRangesProperty(void);
EXTERN void TermRangedRangesProperty(void);

EXTERN void InitRangedWoundProperty(void);
EXTERN void TermRangedWoundProperty(void);

////////////////////////////////////////////////////////////////////////////////

#endif






