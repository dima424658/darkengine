/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostapi.h,v 1.5 2000/01/29 13:41:06 adurant Exp $
#pragma once

#ifndef __GHOSTAPI_H
#define __GHOSTAPI_H

// include the public flags
#include <ghostflg.h>

//////////////////////////////
// add/remove of ghosts

// to setup ghosts into the tables
EXTERN void GhostAddLocal(ObjID obj, float detail, int flags);
EXTERN void GhostAddRemote(ObjID obj, float detail, int flags);

// removal
EXTERN void GhostRemLocal(ObjID obj);
EXTERN void GhostRemRemote(ObjID obj);

//////////////////////////////
// notification of ghosts

// im gonna toast this things physics
EXTERN void GhostNotifyPhysDeregister(ObjID obj);

// are we allowed to send a deregister packet for this obj
EXTERN BOOL GhostAllowDeregisterPacket(ObjID obj);

// general ghost notification - events are eGhostState
EXTERN void GhostNotify(ObjID obj, int event);

// for changing the config... be very very careful
EXTERN void GhostChangeLocalConfig(ObjID obj, int flags, BOOL off);
EXTERN void GhostChangeRemoteConfig(ObjID obj, int flags, BOOL off);

// this object has changed owners, if we care
EXTERN void GhostNotifyOwnerChanged(ObjID obj, BOOL iOwnIt);

// set w/OBJ_NULL to set no weapon
// set w/charging to mean, well, charging
EXTERN void GhostSetWeapon(ObjID ghost, ObjID weapon, BOOL charging);

// remove all weapons for all ghosts (in order to get ready to save to briefcase)
EXTERN void GhostRemoveWeapons(void);

//////////////////////////////
// callbacks set by the app if it wants to do things

// player ghost in mode 0 wants to do "state"
EXTERN int (*GhostPlayerActionCallback)(ObjID ghost, int mode, int state);
// swizzle a weapon objID into a smaller number (1-100 sort of thing)
EXTERN int  (*GhostWeaponObjSwizzle)(ObjID weapon, BOOL charging);
// takes the smaller number from above, attaches to this ghost
//   (should take an pGR, but that would require that typedef, so screw it)
EXTERN void (*GhostWeaponObjAttach)(ObjID ghost, int weapon);
// called when the "firing" flag on a player mocap is hit
EXTERN void (*GhostInstallFireListenFunc)(ObjID obj);

//////////////////////////////
// per frame

// actually process the frame
EXTERN void GhostFrame(int dt);

//////////////////////////////
// init/term

EXTERN void GhostInit(void);
EXTERN void GhostTerm(void);
EXTERN void GhostDBReset(void);

#endif  // __GHOSTAPI_H
