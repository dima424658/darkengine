/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mschutil.h,v 1.6 2000/01/29 13:22:18 adurant Exp $
#pragma once

#ifndef __MSCHUTIL_H
#define __MSCHUTIL_H

#ifdef __cplusplus

#include <motdbtyp.h>
#include <mschtype.h>

// motion schema utilities for getting data associated with schemas and their
// archetypes, including game-assigned data.  This is a "glue" file, and different
// apps may want to have different versions.

EXTERN BOOL MSchUGetGaitData(const cMotionSchema *pSchema,sMGaitSkillData **pData);

EXTERN BOOL MSchUGetSwordActionType(const cMotionSchema *pSchema, int *pType);

EXTERN BOOL MSchUGetMotControllerID(const cMotionSchema *pSchema, int *pID);

EXTERN BOOL MSchUGetPlayerSkillData(const cMotionSchema *pSchema, sMPlayerSkillData *pSkillData);

EXTERN BOOL MSchUGetPlayerPosOffset(const cMotionSchema *pSchema, mxs_vector *pPosOff, mxs_angvec *pAngOff);

#endif // cplusplus

#endif
