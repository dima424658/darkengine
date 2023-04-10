/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aivision.h,v 1.10 1998/10/28 11:51:47 TOML Exp $
//
//
//

#ifndef __AIVISION_H
#define __AIVISION_H

#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIVisionCone
//

enum eAIVisonConeFlags
{
   kAIVC_Active   =  0x01,
   kAIVC_NoAlert0 =  0x02,
   kAIVC_NoAlert1 =  0x04,
   kAIVC_NoAlert2 =  0x08,
   kAIVC_NoAlert3 =  0x10,

   kAIVC_AlertnessRestricted = (kAIVC_NoAlert0 | kAIVC_NoAlert1 | kAIVC_NoAlert2 | kAIVC_NoAlert3),

   kAIVC_Periph   =  0x20,
   kAIVC_Omni     =  0x40,
   kAIVC_LowLight =  0x80,

   kAIVC_Behind   = 0x100,
};

///////////////////////////////////////

struct sAIVisionCone
{
   // Flags
   unsigned flags;
   
   // Angle of the cone in XY
   int angle;
   
   // Angle of the cone in Z
   int zAngle;
   
   // Range of the cone
   int range;
   
   // Overall sensitivity (pct)
   int acuity;
};

///////////////////////////////////////
//
// STRUCT: sAIVisionDesc
//

#define kAIMaxVisionCones 10

struct sAIVisionDesc
{
   // For future use
   DWORD reserved[8];
   
   // Cones, in the order that they should be evaluated
   sAIVisionCone cones[kAIMaxVisionCones];
   
   // Z offset from base pos that AI looks from
   float zOffset; 
};

#define AI_VISDESC_RESERVED { 0, 0, 0, 0, 0, 0, 0, 0 }
#define AI_NULL_VISCONE     { 0, 0, 0, 0, 0 }

///////////////////////////////////////

extern sAIVisionDesc g_AIDefVisionDesc;

///////////////////////////////////////
//
// STRUCT: sAIVibilityMods
//

#define kAIMaxVisibilityMods 6

struct sAIVisibilityMods
{
   float m_mods[kAIMaxVisibilityMods];
};

extern sAIVisibilityMods g_AIDefVisibilityMods;

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIAlertSenseMults
//

struct sAISenseMult
{
   float XYMult;
   float ZMult;
   float RMult;
   float KnowledgeMult;
};

///////////////////////////////////////

struct sAIAlertSenseMults
{
   sAISenseMult mults[kAIAL_Num];
   float        CombatKnowledgeMult;
};

extern sAIAlertSenseMults g_AIDefAlertSenseMults;

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIVisibility
//
// Describes how visible an object is
//

struct sAIVisibility
{
   // Composite visibility level
   int   level;
   
   // Source data from which above is derived
   int   lighting;
   int   movement;
   int   exposure;
   
   // Time the visibility data was last refreshed
   ulong updateTime;
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIVisibilityControl
//

struct sAIVisibilityControl
{
   int lowVisibility;
   int midVisibility;
   int highVisibility;
   
   float lowSpeed;
   float highSpeed;
  
   int lowSpeedMod;
   int normSpeedMod;
   int highSpeedMod;

   float wallDist;   
   
   int crouchMod;
   int wallMod;
   
   float cornerLeanMod;
};

///////////////////////////////////////

extern sAIVisibilityControl g_AIDefVisCtrl;

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIVISION_H */
