/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plycbllm.h,v 1.18 2000/01/31 10:00:20 adurant Exp $
#pragma once

#ifndef __PLYCBLLM_H
#define __PLYCBLLM_H

#include <objtype.h>
#include <property.h> // for listener structs

#ifdef __cplusplus
extern "C" {
#endif // ifdef cplusplus

#include <plyrtype.h>
#include <mschtype.h>

EXTERN void LGAPI PlayerCreatureListener(sPropertyListenMsg* msg, PropListenerData data);

// control input funcs
//
EXTERN void PlayerStartAction();
EXTERN void PlayerStartActionOfType(tPlayerActionType type);
EXTERN void PlayerFinishAction();
EXTERN void PlayerAbortAction();
EXTERN void PlayerSwitchMode(int mode);
EXTERN int  PlayerGetMode();
EXTERN int  PlayerGetNextMode();

EXTERN ObjID PlayerArm();

// schema utility funcs
//
EXTERN BOOL PlayerGetMotControllerID(int *pID);
EXTERN BOOL PlayerGetSkillData(sMPlayerSkillData *pSkillData);

// cerebellum setup/loop manager funcs
//
EXTERN void PlayerCbllmInit();

EXTERN void PlayerCbllmSetFrameCallback(fPlayerFrameCallback callback);

EXTERN void PlayerCbllmSetEnterModeCallback(fPlayerModeChangeCallback callback);
EXTERN void PlayerCbllmSetLeaveModeCallback(fPlayerModeChangeCallback callback);
#ifdef __cplusplus
EXTERN void PlayerCbllmSetAbilityCallback(fPlayerAbilityCallback callback);
#endif

EXTERN void PlayerCbllmSetIdleCallback(fPlayerIdleCallback callback);

EXTERN void PlayerCbllmCreate();

EXTERN void PlayerCbllmDestroy();

EXTERN void PlayerCbllmUpdate(ulong dt);

EXTERN BOOL PlayerCbllmGetPower(float *pPower);


// for debugging/tweaking purposes
#ifndef SHIP
EXTERN void PlayerArmSetMocap(char *motionName);
EXTERN void PlayerCbllmSpewArmOffset();
EXTERN void PlayerCbllmSetArmOffset();
EXTERN void PlayerCbllmSetAllArmOffsets();
EXTERN void PlayerCbllmDetachArm();
#endif


#ifdef __cplusplus
}
#endif // ifdef cplusplus

#endif
