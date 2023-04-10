/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaipr.h,v 1.3 1999/05/23 15:30:53 JON Exp $
//
// AI properties
//

#ifndef __SHKAIPR_H
#define __SHKAIPR_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// Melee target property
//

extern IBoolProperty* g_pAINotMeleeTargetProperty;

#define AIGetNotMeleeTarget(obj) AIGetProperty(g_pAINotMeleeTargetProperty, (obj), (BOOL)FALSE)

void ShockAIInitMeleeTargetProp(void);
void ShockAITermMeleeTargetProp(void);

///////////////////////////////////////////////////////////////////////////////
//
// Swarm Ability property
//

F_DECLARE_INTERFACE(IAISwarmProperty);

///////////////////////////////////////////////////////////////////////////////

void ShockAIInitSwarmProp(void);
void ShockAITermSwarmProp(void);

///////////////////////////////////////

#define PROP_AI_SWARM "AI_Swarm"

EXTERN IAISwarmProperty * g_pAISwarmProperty;

struct sAISwarmParams
{
   float m_closeDist;   // distance to close to before backing off
   float m_backOffDist; // distance to back off to
};

EXTERN const sAISwarmParams g_AIDefaultSwarmParams;

#define ShockAIGetSwarmParams(obj) \
   AIGetProperty(g_pAISwarmProperty, (obj), (sAISwarmParams *)&g_AIDefaultSwarmParams)

EXTERN float ShockAIGetCloseDist(ObjID objID);
EXTERN float ShockAIGetBackOffDist(ObjID objID);

#undef  INTERFACE
#define INTERFACE IAISwarmProperty
DECLARE_PROPERTY_INTERFACE(IAISwarmProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISwarmParams *);
};

///////////////////////////////////////////////////////////////////////////////
//
// GrubCombat Ability property
//

F_DECLARE_INTERFACE(IAIGrubCombatProperty);

///////////////////////////////////////////////////////////////////////////////

void ShockAIInitGrubCombatProp(void);
void ShockAITermGrubCombatProp(void);

///////////////////////////////////////

#define PROP_AI_GRUB_COMBAT "AI_Grub_Combat"

EXTERN IAIGrubCombatProperty* g_pAIGrubCombatProperty;

struct sAIGrubCombatParams
{
   float m_leapDist;    // distance at which to bite
   float m_biteDist;    // distance at which to bite
   char m_stimulus[32]; // stimulus delivered
   float m_intensity;   // stim intensity
   float m_leapSpeedX;    
   float m_leapSpeedZ;
   unsigned m_minLeapTime; // min time between leaps
   unsigned m_maxLeapTime; // max time between leaps (if able to)
};

EXTERN const sAIGrubCombatParams g_AIDefaultGrubCombatParams;

#define ShockAIGetGrubCombatParams(obj) \
   AIGetProperty(g_pAIGrubCombatProperty, (obj), (sAIGrubCombatParams *)&g_AIDefaultGrubCombatParams)

EXTERN float ShockAIGetBiteDist(ObjID objID);
EXTERN const char* ShockAIGetBiteStimulus(ObjID objID);
EXTERN float ShockAIGetBiteIntensity(ObjID objID);

#undef  INTERFACE
#define INTERFACE IAIGrubCombatProperty
DECLARE_PROPERTY_INTERFACE(IAIGrubCombatProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIGrubCombatParams *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Wander Ability
//

F_DECLARE_INTERFACE(IAIWanderProperty);

///////////////////////////////////////////////////////////////////////////////

void ShockAIInitWanderProp(void);
void ShockAITermWanderProp(void);

///////////////////////////////////////

#define PROP_AI_Wander "AI_Wander"

EXTERN IAIWanderProperty * g_pAIWanderProperty;

struct sAIWanderParams
{
   float m_wanderDist;   // max distance to wander
};

EXTERN const sAIWanderParams g_AIDefaultWanderParams;

#define ShockAIGetWanderParams(obj) \
   AIGetProperty(g_pAIWanderProperty, (obj), (sAIWanderParams *)&g_AIDefaultWanderParams)

EXTERN float ShockAIGetWanderDist(ObjID objID);

#undef  INTERFACE
#define INTERFACE IAIWanderProperty
DECLARE_PROPERTY_INTERFACE(IAIWanderProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIWanderParams *);
};

#pragma pack()

#endif /* !__SHKAIPR_H */



