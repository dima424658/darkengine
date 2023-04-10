/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objpos.h,v 1.6 2000/01/31 10:00:14 adurant Exp $
#pragma once

#ifndef __OBJPOS_H
#define __OBJPOS_H

#include <objtype.h>
#include <matrixs.h>

////////////////////////////////////////////////////////////
// OBJECT POSITION WRAPPER 
//

typedef struct Position ObjPos;

//----------------------------------------
// ACCESSORS
//

// Get an object's position
EXTERN ObjPos* ObjPosGet(ObjID obj);

// if you just want the location vector
EXTERN mxs_vector *ObjPosGetLocVector(ObjID obj);

// Must be concrete and in range and all that 
EXTERN ObjPos* ObjPosGetUnsafe(ObjID obj);

// unset an objpos
EXTERN void ObjPosUnset(ObjID obj);

// Update position, and reref if necessary
// Note that if you have a valid Position already
// the interface ObjPosCopyUpdate is preferred
EXTERN void ObjPosUpdate(ObjID obj, const mxs_vector* pos, const mxs_angvec* ang);

// Update position, must be concrete. 
// Note that if you have a valid Position already
// the interface ObjPosCopyUpdateUnsafe is preferred
EXTERN void ObjPosUpdateUnsafe(ObjID obj, const mxs_vector* pos, const mxs_angvec* ang);

// if you want to move ObjPos to objpos w/o breaking down into vectors;
// but you must have used the wrtype macros to build a valid Position
EXTERN void ObjPosCopyUpdate(ObjID obj, ObjPos *vp);

// Just update position or orientation, not breakneck fast. 
// If you have a valid Location (e.g. from a raycast), the
// interface ObjPosSetLocation is preferred, as it preserves
// all the Location information.
EXTERN void ObjRotate(ObjID obj, const mxs_angvec* ang); 
EXTERN void ObjTranslate(ObjID obj, const mxs_vector* loc); 

// Just update the position, but using the Location interface--
// you must have used the wrtype macros to build a valid Location
EXTERN void ObjPosSetLocation(ObjID obj, const struct Location *newloc);

// TO USE THIS YOU NEED TO INCLUDE wrobj.h 
#define ObjPosCopyUpdateUnsafe(obj,vp) ObjPosUpdateUnsafe(obj,&vp->loc.vec,&vp->fac);

// Force an object to re-ref right now.
EXTERN void ObjForceReref(ObjID obj);

// Unref & reref all objects
EXTERN void ObjDeleteAllRefs(void);
EXTERN void ObjBuildAllRefs(void);

//----------------------------------------
// Notification 
//

typedef void (*ObjPosCB)(ObjID obj, const ObjPos* pos, void* data); 

// Subscribe to callbacks when positions change
EXTERN void ObjPosListen(ObjPosCB cb, void* data);

// Reref all "dirty" objects & call callbacks 
EXTERN void ObjPosSynchronize(void);

// "dirty" an object without moving it 
EXTERN void ObjPosTouch(ObjID obj);


//----------------------------------------
// Initialization 
//

// Init/Term object position subsystem
EXTERN void ObjPosInit(void);
EXTERN void ObjPosTerm(void); 


#endif // __OBJPOS_H






