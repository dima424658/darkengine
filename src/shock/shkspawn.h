/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkspawn.h,v 1.7 2000/01/31 09:59:15 adurant Exp $
#pragma once

#ifndef __SHKSPAWN_H
#define __SHKSPAWN_H

typedef enum 
{
   kEcologyNormal = 0,
   kEcologyHacked = 1,
   kEcologyAlert = 2,
};

#define MAX_SPAWN_POINTS   32

typedef enum
{
   kSpawnFlagNone = 0x0,
   kSpawnFlagPopLimit = 0x1,
   kSpawnFlagPlayerDist = 0x2,
   kSpawnFlagGotoAlarm = 0x4,
   kSpawnFlagSelfMarker = 0x8,
   kSpawnFlagRaycast = 0x10,
   kSpawnFlagFarthest = 0x20,

   kSpawnFlagDefault = kSpawnFlagPopLimit|kSpawnFlagPlayerDist,
   kSpawnFlagAll  = 0xFFFF,
};

// ecology info, for controlling behavior of an ecology trap
typedef struct sEcologyInfo
{
   float m_period;
   int m_mincount[3];
   int m_maxcount[3];
   float m_recovery[3];
   int m_randval[3];
} sEcologyInfo;

// EcologyInfo description property
F_DECLARE_INTERFACE(IEcologyInfoProperty);

#undef INTERFACE
#define INTERFACE IEcologyInfoProperty

DECLARE_PROPERTY_INTERFACE(IEcologyInfoProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sEcologyInfo*);  // Type-specific accessors, by reference
};

#define PROP_ECOLOGY_NAME "Ecology"
EXTERN IEcologyInfoProperty* gPropEcology;

#define PROP_ECOTYPE_NAME "EcoType"
EXTERN IIntProperty *gPropEcoType;

#define PROP_ECOSTATE_NAME "EcoState"
EXTERN IIntProperty *gPropEcoState;

// spawn info, for object generation traps
#define MAX_SPAWN_TYPES    4
typedef struct sSpawnInfo
{
   char m_objs[MAX_SPAWN_TYPES][64];
   int m_odds[MAX_SPAWN_TYPES];
   int m_flags;
   int m_supply;
} sSpawnInfo;

// SpawnInfo description property
F_DECLARE_INTERFACE(ISpawnInfoProperty);

#undef INTERFACE
#define INTERFACE ISpawnInfoProperty

DECLARE_PROPERTY_INTERFACE(ISpawnInfoProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sSpawnInfo*);  // Type-specific accessors, by reference
};

#define PROP_SPAWN_NAME "Spawn"
EXTERN  ISpawnInfoProperty* gPropSpawn;
EXTERN  IFloatProperty *gPropHackVisibility;

#endif