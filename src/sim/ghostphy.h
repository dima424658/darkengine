/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostphy.h,v 1.10 1999/08/26 12:52:34 dc Exp $
// physics functions for ghosts

#ifndef __GHOSTPHY_H
#pragma once
#define __GHOSTPHY_H

#include <ghosttyp.h>

// turn on/off "jump", ie. gravity and friction
EXTERN void _GhostJumpOn(ObjID obj);
EXTERN void _GhostJumpOff(ObjID obj);

// horrible hacks to go after efficiency, etc
EXTERN BOOL _GhostHaveGrav(ObjID obj);
EXTERN BOOL _GhostGravRemote(ObjID obj, sGhostPos *pPos, int cfg_flags);
EXTERN BOOL _GhostGravLocal(ObjID obj, sGhostPos *pPos, int cfg_flags);
// may i just say i hate this - have to rewrite using the callbacks
EXTERN void _GhostFixupGrav(sGhostRemote *pGR);

// Fix up remote ghosts physics
EXTERN void _GhostRemoteRelObjPhys(ObjID ghost, ObjID rel_obj, ObjID last_rel_obj);

// put me to sleep, physically
EXTERN void _GhostPhysSleep(sGhostRemote *pGR);

// find me a valid loc, given where we last were and where we want to be
EXTERN void _GhostFindValidLoc(ObjID ghost, mxs_vector *pDesired, mxs_vector *pLast, mxs_vector *pResult);

// the endless idiocy of our starting and killing our object N times on startup
EXTERN void _GhostSetupInitialPhysWeap(sGhostRemote *pGR);

// initialize physics right when we're created
EXTERN void _GhostSetupInitialPhysRemote(sGhostRemote *pGR);
EXTERN void _GhostSetupInitialPhysLocal(sGhostLocal *pGR);

// configure phys parameters for a remote model
EXTERN void _GhostConfigureRemoteModels(sGhostRemote *pGR);
// actually build the motor controller etc etc... (and go ballistic)
EXTERN void _GhostBuildMotionCoord(sGhostRemote *pGR);
// TRUE iff the ghost is ready for us to build it
EXTERN BOOL _GhostAllowedToBuildModels(sGhostRemote *pGR);
// put the object's physics back to what they would normally be,
// for something that used to be a remote ghost
EXTERN void _GhostRepairRemoteModels(sGhostRemote *pGR);

// converstions to and from obj/world space for moving terrain
EXTERN void _ghost_vel_worldtorel(ObjID rel_obj, mxs_vector *vel);
EXTERN BOOL _ghost_pos_worldtorel(ObjID ghost, ObjID *rel_obj, mxs_vector *pos, int ghost_flags);
EXTERN BOOL _ghost_pos_reltoworld(ObjID ghost, ObjID rel_obj, mxs_vector *pos);
// deal with the height difference in models
EXTERN void _ghost_pos_centerconvert(ObjID ghost, mxs_vector *pos);

// helper to know if your pred vel is facing the ground you are on - assumes you already asked about on
EXTERN BOOL _GhostFacingGround(ObjID obj, const mxs_vector *vel);
// bleed velocity off the predition based on numerous innumerable forces (tm)
EXTERN void _GhostBleedVelocity(ObjID obj, sGhostPos *pPred, int cfg_flags, int last_time, float dt);
// teleports - returns success - if you have the delta, great...
EXTERN BOOL _GhostTeleport(ObjID ghost, mxs_vector *delta, const mxs_vector *target);

// using netPos for control, update ghostPos by dt more dt's
EXTERN void _GhostApproxPhys(ObjID obj, sGhostPos *netPos, sGhostPos *ghostPos, float dt, BOOL gravity);

// helper function to decide if vec is really ====== zero   
EXTERN BOOL _is_zero_vec(const mxs_vector *vec);

#endif  // __GHOSTPHY_H
