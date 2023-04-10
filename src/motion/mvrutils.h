/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrutils.h,v 1.15 2000/01/31 09:49:56 adurant Exp $
#pragma once

#ifndef __MVRUTILS_H
#define __MVRUTILS_H

#include <matrixs.h>
#include <wrtype.h>
#include <math.h> // XXX for fabs

#ifdef __cplusplus
#include <mcoord.h>
#include <dynarray.h>

EXTERN cDynArray<ObjID> standableObjectList;

#endif

typedef struct sMotionPhys
{
   mxs_vector xlat;
   mxs_real distance; // magnitude of xlat vector
   mxs_real duration;
   mxs_real buttZOffset;
   mxs_ang endDir;
} sMotionPhys;

typedef enum eMotionStatus
{
   kMotionStart,
   kMotionEnd,
   kMotionFlagReached,
   kMotionStatusInvalid=0xFFFF,
} eMotionStatus;

#define kMvrUCF_MaintainSpeed 0x1

#ifdef __cplusplus
EXTERN BOOL MvrGetRawMovement(const IMotor *pMotor, const int motionNum, sMotionPhys &phys);

EXTERN void MvrCalcSlidingCollisionResponse(const mxs_vector *pForces, const int nForces, 
                                            const mxs_vector *ovel, mxs_vector *nvel, ulong flags=NULL);
#endif

EXTERN void InitGroundHeightObjects();
EXTERN void TermGroundHeightObjects();

EXTERN BOOL MvrFindGroundHeight(ObjID obj, Location *loc, mxs_real *pHeight, ObjID *pGroundObj);
EXTERN void MvrSetStandingObj(ObjID standingObj, ObjID supportObj);
EXTERN void MvrSetStandingOffset(ObjID standingObj, ObjID supportObj, mxs_vector *pExpectedEndPos);
EXTERN void MvrSetTerrStandingOffset(ObjID standingObj, mxs_vector *pOffset);

EXTERN int  MvrRandomIntInRange(int low, int high);

EXTERN BOOL MvrGetEndButtHeight(const IMotor *pMotor, int motionNum, float *pHeight);

EXTERN void MvrUtilsRegisterDefaultObjFlags(ObjID obj);
EXTERN void MvrUtilsUnRegisterDefaultObjFlags(ObjID obj);

EXTERN void MvrProcessStandardFlags(IMotor *pMotor, cMotionCoordinator *pCoord, const int flags);
EXTERN void MvrSendMotionMessage(ObjID obj, int motionNum, eMotionStatus whatHappened, ulong flags);

#endif





