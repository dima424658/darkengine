/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phclimb.h,v 1.5 2000/01/29 13:32:37 adurant Exp $
//
// Physics Climbing API
//
#pragma once

#ifndef __PHCLIMB_H
#define __PHCLIMB_H

#include <matrixs.h>
#include <objtype.h>

#ifdef __cplusplus
class cPhysModel;
#else
typedef void cPhysModel;
#endif

EXTERN BOOL PhysObjIsClimbing(ObjID objID);
EXTERN BOOL PhysObjIsMantling(ObjID objID);

EXTERN void BreakClimb(ObjID objID, BOOL jumping, BOOL jump_thru);
EXTERN BOOL CheckClimb(ObjID objID);

EXTERN void BreakMantle(ObjID objID);
EXTERN BOOL CheckMantle(ObjID objID);

EXTERN void UpdateMantling(cPhysModel *pModel, mxs_real dt);

EXTERN void GetClimbingDir(ObjID objID, mxs_vector *dir);


#endif
