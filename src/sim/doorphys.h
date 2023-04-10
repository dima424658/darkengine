/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/doorphys.h,v 1.17 2000/02/24 13:44:23 patmac Exp $
// temporary? door system
#pragma once

#ifndef __DOORPHYS_H
#define __DOORPHYS_H

#include <matrixs.h>

#define DOORPHYS_TOGGLE (0)
#define DOORPHYS_OPEN   (1)
#define DOORPHYS_CLOSE  (2)

enum eDoorStatus_
{
   kDoorClosed = 0,
   kDoorOpen,
   kDoorClosing,
   kDoorOpening,
   kDoorHalt,
   kDoorNoDoor,
};
typedef int eDoorStatus;

// slam a door open or closed
EXTERN void DoorSlamOpen(int o_id);
EXTERN void DoorSlamClosed(int o_id);

// activate the door
EXTERN BOOL DoorPhysActivate(int o_id, int what_to_do);

// abort the door's motion
EXTERN void DoorPhysAbort(int o_id);

// get the door's status
EXTERN int  GetDoorStatus(int o_id);

// update the door property from the brush
EXTERN void UpdateDoorBrush(int o_id);

// updates the door's physics from the property
EXTERN void UpdateDoorPhysics(int o_id);

// generate the base (open & closed) locations and store them off
EXTERN void GenerateBaseDoorLocations(int o_id);

// Determine the rooms on each side of the (or all) door(s)
EXTERN void GenerateAllDoorRooms();
EXTERN void GenerateDoorRooms(int o_id);

// Removes a door's blocking effects
EXTERN void RemoveDoorBlocking(int o_id);
// Remove just door's sound blocking effects
EXTERN void RemoveDoorSoundBlocking(int o_id);

// Enable's a door's blocking effects
EXTERN void AddDoorBlocking(int o_id);
// Enable just door's sound blocking effects
EXTERN void AddDoorSoundBlocking(int o_id);
// See if a door is blocking sound
EXTERN BOOL GetDoorSoundBlocking( int o_id );

// Check for whether the player's leaning or unleaning should
// cause any doors to not block sound
EXTERN void CheckDoorLeanSoundBlocking();

EXTERN void UnblockVisibilityForAllOpen();

// is this object a door?
EXTERN BOOL IsDoor(int o_id);

// how big a mass can this door push?
EXTERN mxs_real GetDoorPushMass(int o_id);

// does the door use hard limits?
EXTERN BOOL DoorHardLimits(int o_id);

// callbacks
EXTERN BOOL RotDoorOpenCallback(int o_id);
EXTERN BOOL RotDoorCloseCallback(int o_id);
EXTERN BOOL TransDoorOpenCallback(int o_id);
EXTERN BOOL TransDoorClosedCallback(int o_id);

#endif  // __DOORPHYS_H


