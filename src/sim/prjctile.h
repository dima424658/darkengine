/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/prjctile.h,v 1.20 2000/01/31 10:00:31 adurant Exp $
#pragma once

#ifndef PRJCTILE_H
#define PRJCTILE_H

#include <objtype.h>
#include <matrixs.h>
#include <propface.h>

////////////////////////////////////////////////////////////
// PROJECTILE FIRING API
//


#define PRJ_FLG_ZEROVEL  (1 << 0)  // ignore launcher velocity
#define PRJ_FLG_PUSHOUT  (1 << 1)  // push away from launcher
#define PRJ_FLG_FROMPOS  (1 << 2)  // don't init position (only makes sense for concretes)
#define PRJ_FLG_GRAVITY  (1 << 3)  // object has gravity if default physics
// is there some reason for this gap in the flags here?
#define PRJ_FLG_BOWHACK (1 << 8)  // do all bow hackery
#define PRJ_FLG_TELLAI  (1 << 9)  // tell AIs about this object
#define PRJ_FLG_NOPHYS  (1 <<10)  // create the object without adding physics
#define PRJ_FLG_MASSIVE (1 <<11)  // slow down the velocity based on  
                                  // projectile mass & launcher mass 
#define PRJ_FLG_NO_FIRER (1<<12)  // don't creature firer link

#define launchProjectileDefault(a,b,c) launchProjectile(a,b,c,0,NULL,NULL,NULL)

EXTERN ObjID launchProjectile(ObjID launcher, ObjID projArchetype, float power, int flags, const mxs_vector *add_vel, const mxs_vector *dir, const mxs_vector *loc);

//
// Is this a projectile?
//
EXTERN BOOL ObjIsProjectile(ObjID obj);

//
// Get its firer
//
EXTERN ObjID GetProjectileFirer(ObjID obj);
EXTERN void  SetProjectileFirer(ObjID bullet, ObjID firer);  // firer == OBJ_NULL, break the link

//
// Get arrow archetype to use for creating arrow
//
EXTERN ObjID GetArrowArchetype();

#define RELATION_FIRER_NAME "Firer" 

// Projectile sound property
#define PROP_PROJ_SND_NAME "PrjSound"
EXTERN IStringProperty* g_pProjSound;

//
// Player-based firing commands
//
EXTERN void PlayerStartShooting(void);   // shoot a bullet.
EXTERN void PlayerFinishShooting(void);
EXTERN void PlayerUpdateFiring(void);  // update player firing state (call each frame)

EXTERN void InitProjectileArchetype(void);
EXTERN void InitProjectiles(void);

EXTERN void set_arrow_arch(ObjID arch);

#endif // PRJCTILE_H








