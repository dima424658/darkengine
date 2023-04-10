/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phconst.h,v 1.29 2000/02/23 23:57:54 adurant Exp $
//
// Collected physics constants
//
#pragma once


#ifndef __PHCONST_H
#define __PHCONST_H

///////////////////////////////////////////////////////////////////////////////

//
// Minimum relative velocity below which we stick to a terrain surface
//
const mxs_real kBreakTerrainContactVel = 5.0;

//
// Minimum relative velocity below which we stick to an object
//
const mxs_real kBreakObjectContactVel = 5.0;

//
// Minimum relative velocity above which we destroy all contacts 
//
const mxs_real kBreakAllObjectContactVel = 1000.0;

//
// Distance at which we judge terrain contact to be broken
//
const mxs_real kBreakTerrainContactDist = 0.1;

//
// Distance at which we judge object contact to be broken
//
const mxs_real kBreakObjectContactDist = 0.2;

//
// Distance to backup from a collision (squared)
//
const mxs_real kCollisionBackup2 = 0.01;

//
// Max proportion of distance travelled to backup
//
const mxs_real kCollisionBackupMax = 0.5;

//
// Max number of collisions we allow/frame/model
//
const int kMaxFrameCollisions = 32;

//
// Pump some energy into object vs. object collisions...
//
const mxs_real kObjectCollisionEnergy = 0.0;

//
// Gravity
//
EXTERN mxs_vector kGravityDir;
EXTERN mxs_real   kGravityAmt;

//
// Default terrain elasticity
//
const mxs_real kTerrainBounce = 0.1;

//
// Min velocity magnitude a sliding object can have before it 
// stops (squared). 
//
const mxs_real kMinVelocityMag = 2;

//
// Amount of friction when sliding (0 is ice, 10 is lots-o-friction). 
//
extern mxs_real kFrictionFactor;

//
// Maximum length of physics frame
//
extern int kMaxFrameLen;

//
// Period of being squished
//
const long kSquishPeriod = 500;

//
// Maximum velocity at which player head will spring, unless falling.
//  

EXTERN mxs_real kSpringCapMag;

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHCONST_H */


