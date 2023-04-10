/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/motedit_.h,v 1.2 2000/01/29 13:12:42 adurant Exp $
// internal motion editor header
#pragma once

#ifndef __MOTEDIT__H
#define __MOTEDIT__H

#include <multiped.h>
#include <motedit.h>

#define MROLE_MAIN 0

#define NUM_MOTION_ROLES 1

#if 0
// XXX taken out until further notice KJ 2/98
extern mps_motion_node activeMotions[NUM_MOTION_ROLES];
#endif

// global variables.  bad form, I know
EXTERN BOOL motionIsPlaying;
EXTERN int g_CurMotEditFrame;
EXTERN int g_LastMotEditFrame;
EXTERN char *g_MotEditMotionName;
EXTERN int g_MotEditMotionNum;
EXTERN mps_motion *g_MotEditMotion;
EXTERN mxs_vector g_MotEditStartPos;
EXTERN quat g_MotEditStartRot;

// functions
EXTERN void MotEditUpdate(float dt);
EXTERN BOOL MotEditSetEditMotion(char *motname);
EXTERN void MotEditFinishMotionEdit();

EXTERN void MotEditSetEditObj();

multiped *MotEditGetMultiped();

#ifdef __cplusplus
#include <motrtype.h>
IMotor *MotEditGetMotor();
#endif // cplusplus

#endif

