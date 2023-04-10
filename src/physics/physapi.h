/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/physapi.h,v 1.56 2000/01/31 09:52:12 adurant Exp $
//
// Physics API
//
// @TBD (toml 08-05-97): when the time is right, we'll convert this to a component object
//
// @Note (toml 08-05-97): we'll probably want to expose some base structures here, like
//                        PhysClsn in phytyp2.h, then allow the installation of game
//                        callbacks for control, update, collision handling, etc.
//
#pragma once

#ifndef __PHYSAPI_H
#define __PHYSAPI_H

#include <matrixs.h>

#include <phystyp2.h>
#include <objtype.h>
#include <wrtype.h>
#include <phlistyp.h>

#ifdef __cplusplus
enum eMediaState;
#else
typedef int eMediaState;
#endif

#define PHYSICS_HIGH_MODEL_PROP_NAME "PhysicsHighDetail"
#define PHYSICS_LOW_MODEL_PROP_NAME  "PhysicsLowDetail"

#define PLAYER_RADIUS (1.2)
#define PLAYER_HEIGHT (6.0)

#define PLAYER_HEAD (0)
#define PLAYER_FOOT (1)
#define PLAYER_BODY (2)
#define PLAYER_KNEE (3)
#define PLAYER_SHIN (4)

#define PLAYER_HEAD_POS  ((PLAYER_HEIGHT / 2) - PLAYER_RADIUS)
#define PLAYER_FOOT_POS  (-(PLAYER_HEIGHT / 2))
#define PLAYER_BODY_POS  ((PLAYER_HEIGHT / 2) - (PLAYER_RADIUS * 3))
#define PLAYER_KNEE_POS  (-(PLAYER_HEIGHT * (13.0 / 30.0)))
#define PLAYER_SHIN_POS  (-(PLAYER_HEIGHT * (11.0 / 30.0)))

#define DEFAULT_SPRING_TENSION (0.6)
#define DEFAULT_SPRING_DAMPING (0.02)

#ifdef SHIP
#define ResetObjectWarnings()
#define SetObjectWarned(x)
#define HasObjectWarned(x) FALSE
#else
void ResetObjectWarnings();
void SetObjectWarned(ObjID objID);
BOOL HasObjectWarned(ObjID objID);
#endif

// for when a system is updating physics itself
EXTERN BOOL PosPropLock;  
EXTERN BOOL PhysicsListenerLock;

///////////////////////////////////////////////////////////////////////////////
//
// Initialization, Update, I/O, and Cleanup
//
///////////////////////////////////////////////////////////////////////////////

EXTERN void PhysInit();
EXTERN void PhysTerm();

EXTERN void PhysPostLoad();
EXTERN void PhysUpdate(long dt);

EXTERN void PhysRead(PhysReadWrite func,eObjPartition part);
EXTERN void PhysWrite(PhysReadWrite func, eObjPartition part);

EXTERN void PhysUpdateProperties();

EXTERN void PhysInitializeContacts();

EXTERN void PhysWakeUpAll();

EXTERN BOOL PhysObjMoveDir(ObjID objID, Location *start, Location *end, Location *hit); 

//////////////////////////////////////////////////////////////////////////////
//
// Public Location and Rotation functions
//
//////////////////////////////////////////////////////////////////////////////

EXTERN void PhysGetModLocation(ObjID objID, mxs_vector *loc);
EXTERN void PhysSetModLocation(ObjID objID, mxs_vector *loc);
EXTERN void PhysGetSubModLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *loc);
EXTERN void PhysSetSubModLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *loc);
EXTERN void PhysSetSubModRelLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *offset);

EXTERN void PhysGetModRotation(ObjID objID, mxs_angvec *ang);
EXTERN void PhysSetModRotation(ObjID objID, mxs_angvec *ang);
EXTERN void PhysGetSubModRotation(ObjID objID, tPhysSubModId subModId, mxs_angvec *ang);
EXTERN void PhysSetSubModRotation(ObjID objID, tPhysSubModId subModId, mxs_angvec *ang);


//////////////////////////////////////////////////////////////////////////////
//
// Public Velocity and Rotational Velocity functions
//
//////////////////////////////////////////////////////////////////////////////

EXTERN void PhysAxisSetVelocity(ObjID objID, int axis, mxs_real speed);
EXTERN void PhysSetVelocity(ObjID objID, mxs_vector *velocity);
EXTERN void PhysGetVelocity(ObjID objID, mxs_vector *velocity);

EXTERN void PhysSetRotationalVelocity(ObjID objID, mxs_vector *ang_velocity);
EXTERN void PhysGetRotationalVelocity(ObjID objID, mxs_vector *ang_velocity);
EXTERN void PhysSetSubModRotationalVelocity(ObjID objID, tPhysSubModId subModId, mxs_vector *ang_velocity);
EXTERN void PhysGetSubModRotationalVelocity(ObjID objID, tPhysSubModId subModId, mxs_vector *ang_velocity);


//////////////////////////////////////////////////////////////////////////////
//
// Public Control functions
//
//////////////////////////////////////////////////////////////////////////////

EXTERN void PhysAxisControlVelocity(ObjID objID, int axis, mxs_real speed);
EXTERN void PhysControlVelocity(ObjID objID, mxs_vector *velocity);
EXTERN void PhysStopAxisControlVelocity(ObjID objID, int axis);
EXTERN void PhysStopControlVelocity(ObjID objID);

EXTERN void PhysAxisControlRotationalVelocity(ObjID objID, int axis, mxs_real speed);
EXTERN void PhysAxisControlSubModRotationalVelocity(ObjID objID, tPhysSubModId subModId, int axis, mxs_real speed);
EXTERN void PhysControlRotationalVelocity(ObjID objID, mxs_vector *velocity);
EXTERN void PhysStopControlRotationalVelocity(ObjID objID);

EXTERN void PhysControlLocation(ObjID objID, mxs_vector *location);
EXTERN void PhysControlSubModLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *location);
EXTERN void PhysStopControlLocation(ObjID objID);

EXTERN void PhysControlRotation(ObjID objID, mxs_angvec *rotation);
EXTERN void PhysStopControlRotation(ObjID objID);


//////////////////////////////////////////////////////////////////////////////
//
// Public sphere-specific functions
//
//////////////////////////////////////////////////////////////////////////////

EXTERN BOOL PhysRegisterSphereDefault(ObjID objID);
EXTERN BOOL PhysRegisterSphere(ObjID objID, tPhysSubModId numSubModels, unsigned flags, mxs_real radius);

EXTERN BOOL PhysRegisterSphereHatDefault(ObjID objID);
EXTERN BOOL PhysRegisterSphereHat(ObjID objID, unsigned flags, mxs_real radius);

EXTERN void PhysSetSubModRadius(ObjID objID, tPhysSubModId subModId, mxs_real radius);


//////////////////////////////////////////////////////////////////////////////
//
// Public OBB-specific functions
//
//////////////////////////////////////////////////////////////////////////////

EXTERN BOOL PhysRegisterOBBDefault(ObjID objID);
EXTERN BOOL PhysRegisterOBB(ObjID objID, unsigned flags);


//////////////////////////////////////////////////////////////////////////////
//
// Public generic functions
//
//////////////////////////////////////////////////////////////////////////////

EXTERN BOOL PhysObjHasPhysics(ObjID objID);
EXTERN BOOL PhysObjValidPos(ObjID objID, mxs_vector *delta);
EXTERN BOOL PhysSphereIntersectsDoor(ObjID objID);
EXTERN BOOL PhysObjOnGround(ObjID objID);
EXTERN BOOL PhysObjOnPlatform(ObjID objID);

EXTERN void PhysSetFlag(ObjID objID, int flag, BOOL state);

EXTERN eMediaState PhysGetObjMediaState(ObjID objID);
EXTERN BOOL PhysObjInWater(ObjID objID);

EXTERN void PhysObjGetFlow(ObjID objID, mxs_vector *flow);

EXTERN void PhysGetAABBox(ObjID objID, mxs_vector *minvec, mxs_vector *maxvec);

EXTERN void     PhysSetGravity(ObjID objID, mxs_real gravity);
EXTERN mxs_real PhysGetGravity(ObjID objID);

EXTERN void     PhysSetMass(ObjID objID, mxs_real mass);
EXTERN mxs_real PhysGetMass(ObjID objID);

EXTERN void     PhysSetElasticity(ObjID objID, mxs_real elasticity);
EXTERN mxs_real PhysGetElasticity(ObjID objID);

EXTERN void     PhysSetDensity(ObjID objID, mxs_real density);
EXTERN mxs_real PhysGetDensity(ObjID objID);

EXTERN void     PhysSetSubModSpringTension(ObjID objID, tPhysSubModId subModId, mxs_real tension);
EXTERN mxs_real PhysGetSubModSpringTension(ObjID objID, tPhysSubModId subModId);

EXTERN void     PhysSetSubModSpringDamping(ObjID objID, tPhysSubModId subModId, mxs_real damping);
EXTERN mxs_real PhysGetSubModSpringDamping(ObjID objID, tPhysSubModId subModId);

EXTERN ObjID PhysGetClimbingObj(ObjID climber);

EXTERN BOOL PhysObjIsRope(ObjID objID);
EXTERN void PhysHitRope(ObjID objID, int magnitude);

EXTERN void PhysExplode(mxs_vector *location, mxs_real magnitude, mxs_real radius_squared);

EXTERN void PhysCreateDefaultPlayer(ObjID objID);

EXTERN void PhysDeregisterModel(ObjID objID);
EXTERN void PhysDeregisterAllModels(void);

EXTERN BOOL PhysGetFriction(ObjID objID, mxs_real *friction);
EXTERN void PhysGetBaseFriction(ObjID objID, mxs_real *base_friction);
EXTERN void PhysSetBaseFriction(ObjID objID, mxs_real  base_friction);

EXTERN void PhysPlayerJump(ObjID player, mxs_real jump_speed);

EXTERN void PhysWakeUpAll();
EXTERN BOOL PhysIsSleeping(ObjID objID);

EXTERN void PhysSpewInfo(ObjID objID);
EXTERN void PhysSpewPlayer();

EXTERN void PhysSetProjectile(ObjID objID);
EXTERN BOOL PhysIsProjectile(ObjID objID);

EXTERN void LockAttachmentObj(ObjID objID);

//////////////////////////////////////////////////////////////////////////////
//
// Listeners
//
//////////////////////////////////////////////////////////////////////////////

EXTERN PhysListenerHandle PhysCreateListener(const char* pName, PhysListenMsgSet set, PhysListenFunc func, PhysListenerData data);
EXTERN void PhysDestroyListener(PhysListenerHandle handle);

EXTERN void PhysListen(PhysListenerHandle handle, ObjID objID);
EXTERN void PhysUnlisten(PhysListenerHandle handle, ObjID objID);
EXTERN void PhysResetListeners(void);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHYSAPI */



