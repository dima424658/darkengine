/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/motion/mvrflags.h,v 1.6 2000/01/31 09:49:54 adurant Exp $
//
#pragma once

#ifndef __MVRFLAGS_H
#define __MVRFLAGS_H

#include <objtype.h>

#ifdef __cplusplus
class IMotor;
class cMotionCoordinator;
#else
typedef void IMotor;
typedef void cMotionCoordinator;
#endif

////////////////////////////////////////////////////////////////////////////////

// Common flags
#define MF_STANDING        0x00000001
#define MF_LEFT_FOOTFALL   0x00000002
#define MF_RIGHT_FOOTFALL  0x00000004
#define MF_LEFT_FOOTUP     0x00000008
#define MF_RIGHT_FOOTUP    0x00000010
#define MF_FIRE_RELEASE    0x00000020
#define MF_CAN_INTERRUPT   0x00000040
#define MF_START_MOT       0x00000080
#define MF_END_MOT         0x00000100

// Flags subject to game-specific interpretation
#define MF_TRIGGER1        0x00001000
#define MF_TRIGGER2        0x00002000
#define MF_TRIGGER3        0x00004000
#define MF_TRIGGER4        0x00008000

#define MF_TRIGGER5        0x00010000
#define MF_TRIGGER6        0x00020000
#define MF_TRIGGER7        0x00040000
#define MF_TRIGGER8        0x00080000


#define MF_FEET_FLAGS (MF_LEFT_FOOTFALL | MF_RIGHT_FOOTFALL | MF_LEFT_FOOTUP | MF_RIGHT_FOOTUP)

#define NEITHER_FOOT  0
#define LEFT_FOOT     1
#define RIGHT_FOOT    2

#define MF_MAX_BITS  32

////////////////////////////////////////////////////////////////////////////////

typedef void (*tFlagListenFunc)(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags);

struct sFlagListen
{
   ObjID objID;
   ulong interestedFlags;
   ulong triggeredFlags;
   tFlagListenFunc fnFlagListeners[32];
};

////////////////////////////////////////////////////////////////////////////////

EXTERN void  MotionFlagsInit();
EXTERN void  MotionFlagsTerm();

EXTERN void  ClearMotionFlagListeners(ObjID obj);
EXTERN void  AddMotionFlagListener(ObjID objID, ulong flags, tFlagListenFunc ListenerFunc);
EXTERN void  RemoveMotionFlagListener(ObjID objID, ulong flags);

EXTERN void  NotifyMotionFlags(ObjID objID, ulong flags, IMotor *pMotor, cMotionCoordinator *pCoord);
EXTERN ulong PollMotionFlags(ObjID objID, ulong flags);

////////////////////////////////////////////////////////////////////////////////

#endif // __MVRFLAGS_H






