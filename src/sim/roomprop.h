/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomprop.h,v 1.11 2000/01/31 10:00:37 adurant Exp $
//
// Room-specific properties
//
#pragma once

#ifndef __ROOMPROP_H
#define __ROOMPROP_H

#include <rect.h>   // for Point

#include <property.h>
#include <propface.h>

///////////////////////////////////////////////////////////////////////////////
//
// Acoustics of room
//

#define PROP_ACOUSTICS_NAME "Acoustics"

typedef struct
{
   int SecretEAXReverbConstant;
   int Dampening;
   int Height;
} sAcousticsProperty;

#undef  INTERFACE
#define INTERFACE IAcousticsProperty
DECLARE_PROPERTY_INTERFACE(IAcousticsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAcousticsProperty*);
};

EXTERN void AcousticsPropInit(void);
EXTERN void AcousticsPropTerm(void);

EXTERN int GetGameSysSecretEAXVar(void);
EXTERN int GetMissionSecretEAXVar(void);

EXTERN const int  g_nRoomTypes;
EXTERN const char *g_RoomTypes[];

///////////////////////////////////////////////////////////////////////////////
//
// Ambient sound in room
//

#define PROP_AMBIENT_NAME "Ambient"

typedef struct
{
   char Name[64];     // @TBD: How long?  Are schemas Labels?
   int  Volume;
} sAmbientProperty;

#undef  INTERFACE
#define INTERFACE IAmbientProperty
DECLARE_PROPERTY_INTERFACE(IAmbientProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAmbientProperty*);
};

EXTERN void AmbientPropInit(void);
EXTERN void AmbientPropTerm(void);

///////////////////////////////////////////////////////////////////////////////
//
// Gravity  0n room
//

#define PROP_ROOMGRAVITY_NAME "RoomGrav"

EXTERN void RoomGravPropInit(void);
EXTERN void RoomGravPropTerm(void);

EXTERN float GetRoomGravity(ObjID objID);


void LoudRoomInit();
void LoudRoomTerm();


///////////////////////////////////////////////////////////////////////////////

EXTERN IAcousticsProperty   *gAcousticsProperty;
EXTERN IAmbientProperty     *gAmbientProperty;
EXTERN IIntProperty         *gRoomGravityProperty;
EXTERN IFloatProperty       *g_pLoudRoomProperty;

///////////////////////////////////////////////////////////////////////////////

#endif











