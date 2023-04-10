/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/creatext.h,v 1.23 2000/01/31 09:50:06 adurant Exp $
// external command file for creature module.
// used for calling from .c files
#pragma once

#ifndef __CREATEXT_H
#define __CREATEXT_H

#include <objtype.h>
#include <mms.h>
#include <matrixs.h>
#include <qt.h>
#include <stdlib.h>
#include <crettype.h>


// XXX can get rid of this if go through property system
typedef void (*fCreatureReadWrite) (void *buf, size_t elsize, size_t nelem);

#ifdef __cplusplus
EXTERN void CreaturesInit(int nCreatureTypes, const sCreatureDesc **ppCreatureDescs, const char **pCreatureTypeNames, const cCreatureFactory **ppCreatureFactories);

EXTERN cCreature *CreatureCreate(int type, const ObjID objID);
#endif

EXTERN void CreaturesClose();
EXTERN void CreaturesUpdate(const ulong timeDelta);

EXTERN void CreaturesRead(fCreatureReadWrite func, eObjPartition partition);
EXTERN void CreaturesWrite(fCreatureReadWrite func, eObjPartition partition);

EXTERN BOOL CreatureExists(const ObjID objID);
// returns FALSE if creature does not exist
EXTERN BOOL CreatureSelfPropelled(const ObjID objID);

EXTERN void CreatureFreeze(ObjID objID);

EXTERN void CreatureUnFreeze(ObjID objID);

EXTERN BOOL CreatureGetRenderHandle(const ObjID objID, long *pHandle);

EXTERN BOOL CreatureGetHeadTransform(ObjID objID, mxs_trans *pHead);

EXTERN void CreatureMeshJointPosCallback(const mms_model *m,const int jointID, mxs_trans *pTrans);
EXTERN void CreatureMeshStretchyJointCallback(const mms_model *m, const int jointID, mxs_trans *pTrans, quat *pRot);

EXTERN void CreatureAbortWeaponMotion(ObjID creature, ObjID weapon);

EXTERN void GetSubModelPos(ObjID objID, int submod, mxs_vector *pPos);

EXTERN void CreaturePrepareToDie(ObjID creature);
EXTERN void CreatureBeDead(ObjID creature);

enum eCMB_Styles
{
   kCMB_Normal,       // Frozen in place and shrunken
   kCMB_Efficient,    // Just frozen, and rotation controlled
   kCMB_WithFeet,     // Frozen with feet so it can stand
   kCMB_Posed,        // Simply ballistic, no other changes
   kCMB_Compressed,   // Physics all at objpos (so you can rotate around all axes)
   kCMB_Corpse        // One shrunked physics model at objpos, with rest axis
};

EXTERN BOOL CreatureMakeBallistic(ObjID creature, int style);
EXTERN void CreatureMakeNonBallistic(ObjID creature);

EXTERN void CreatureMakePhysical(ObjID creature);
EXTERN void CreatureMakeNonPhysical(ObjID creature);

EXTERN void CreatureSetFocusObj(ObjID creature, ObjID focusObj);
EXTERN void CreatureSetFocusLoc(ObjID creature, const mxs_vector *dir);
EXTERN void CreatureSetNoFocus(ObjID creature);

EXTERN void CreaturePosPropLock();
EXTERN void CreaturePosPropUnlock();

#endif
