/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// AI properties
//

#ifndef __DPCAIPR_H
#define __DPCAIPR_H

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

void DPCAIInitMeleeTargetProp(void);
void DPCAITermMeleeTargetProp(void);

///////////////////////////////////////////////////////////////////////////////
//
// DogCombat Ability property
//

F_DECLARE_INTERFACE(IAIDogCombatProperty);

///////////////////////////////////////////////////////////////////////////////

void DPCAIInitDogCombatProp(void);
void DPCAITermDogCombatProp(void);

///////////////////////////////////////

#define PROP_AI_DOG_COMBAT "AI_Dog_Combat"

EXTERN IAIDogCombatProperty* g_pAIDogCombatProperty;

struct sAIDogCombatParams
{
   float    m_leapDist;     // distance at which to bite
   float    m_biteDist;     // distance at which to bite
   char     m_stimulus[32]; // stimulus delivered
   float    m_intensity;    // stim intensity
   float    m_leapSpeedX;    
   float    m_leapSpeedZ;   
   unsigned m_minLeapTime;  // min time between leaps
   unsigned m_maxLeapTime;  // max time between leaps (if able to)
};

EXTERN sAIDogCombatParams g_AIDefaultDogCombatParams;

#define DPCAIGetDogCombatParams(obj) \
   AIGetProperty(g_pAIDogCombatProperty, (obj), (sAIDogCombatParams *)&g_AIDefaultDogCombatParams)

EXTERN float       DPCAIGetBiteDist     (ObjID objID);
EXTERN const char* DPCAIGetBiteStimulus (ObjID objID);
EXTERN float       DPCAIGetBiteIntensity(ObjID objID);

#undef  INTERFACE
#define INTERFACE IAIDogCombatProperty
DECLARE_PROPERTY_INTERFACE(IAIDogCombatProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIDogCombatParams *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Wander Ability
//

F_DECLARE_INTERFACE(IAIWanderProperty);

///////////////////////////////////////////////////////////////////////////////

void DPCAIInitWanderProp(void);
void DPCAITermWanderProp(void);

///////////////////////////////////////

#define PROP_AI_Wander "AI_Wander"

EXTERN IAIWanderProperty * g_pAIWanderProperty;

struct sAIWanderParams
{
   float m_wanderDist;   // max distance to wander
};

EXTERN const sAIWanderParams g_AIDefaultWanderParams;

#define DPCAIGetWanderParams(obj) \
   AIGetProperty(g_pAIWanderProperty, (obj), (sAIWanderParams *)&g_AIDefaultWanderParams)

EXTERN float DPCAIGetWanderDist(ObjID objID);

#undef  INTERFACE
#define INTERFACE IAIWanderProperty
DECLARE_PROPERTY_INTERFACE(IAIWanderProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIWanderParams *);
};

#pragma pack()

#endif /* !__DPCAIPR_H */
