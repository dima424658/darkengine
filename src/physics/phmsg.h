/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmsg.h,v 1.7 2000/01/29 13:33:00 adurant Exp $
//
#pragma once

#ifndef __PHMSG_H
#define __PHMSG_H

#include <objtype.h>
#include <matrixs.h>

class cPhysClsn;
class cPhysModel;

EXTERN void PhysMessageFellAsleep(ObjID object);
EXTERN void PhysMessageWokeUp(ObjID object);

EXTERN void PhysMessageMadePhysical(ObjID object);
EXTERN void PhysMessageMadeNonPhysical(ObjID object);

EXTERN void PhysMessageCollision(cPhysClsn *pClsn, mxs_real magnitude, int *reply1, int *reply2);

EXTERN void PhysMessageContactCreate(cPhysModel *pModel, int submod, int type, ObjID contact_object, int contact_submod);
EXTERN void PhysMessageContactDestroy(cPhysModel *pModel, int submod, int type, ObjID contact_object, int contact_submod);

EXTERN void PhysMessageEnter(cPhysModel *pModel, ObjID trans_object, int trans_submod);
EXTERN void PhysMessageExit(cPhysModel *pModel, ObjID trans_object, int trans_submod);


#endif // __PHSMSG_H
