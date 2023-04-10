/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motprop.h,v 1.8 2000/01/29 13:22:17 adurant Exp $
#pragma once

#ifndef __MOTPROP_H
#define __MOTPROP_H

#include <property.h>
#include <mschtype.h>
#include <label.h>
#include <mclntapi.h>

// This defines all of the motion system properties - actor, motor controller,
// phys characteristics etc.

// actor taglist

#define PROP_ACTOR_TAG_LIST_NAME "MotActorTagList"

#define kActorTagStringSize 800

struct sMotActorTagList
{
   char m_TagStrings[kActorTagStringSize];
};


F_DECLARE_INTERFACE(IMotActorTagListProperty);
#undef INTERFACE
#define INTERFACE IMotActorTagListProperty

DECLARE_PROPERTY_INTERFACE(IMotActorTagListProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sMotActorTagList*);
};


// 
// 'Motor Controller' Property
//

#define PROP_MOTOR_CONTROLLER_NAME "MotorController"

typedef int eMotorControllerType; // actual enum is app-specific

//
//  TimeWarp property
//

#define PROP_TIMEWARP_NAME "TimeWarp"

//
//   SwordAction property 
//

#define PROP_SWORD_ACTION_NAME "SwordActionType"

//
// Player Limbs Position Offsets Property
//

#define PROP_PLAYER_LIMB_OFFSETS_NAME "MotPlyrLimbOff"

typedef struct {
   mxs_vector armPosOff;
   mxs_vector armAngDegrees;
} sMPlayerLimbOffsets;

F_DECLARE_INTERFACE(IMotPlayerLimbOffsetsProperty);
#undef INTERFACE
#define INTERFACE IMotPlayerLimbOffsetsProperty

DECLARE_PROPERTY_INTERFACE(IMotPlayerLimbOffsetsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sMPlayerLimbOffsets*);
};


// 
// 'Phys Limitations' Property
//

#define PROP_MOT_PHYS_LIMITS_NAME "MotPhysLimits"

// XXX TO DO: make this use something more real
typedef struct {
   int foo[2];
} sMotPhysLimits;

F_DECLARE_INTERFACE(IMotPhysLimitsProperty);
#undef INTERFACE
#define INTERFACE IMotPhysLimitsProperty

DECLARE_PROPERTY_INTERFACE(IMotPhysLimitsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sMotPhysLimits*);
};



//
// 'Gait Description' Property
//

#define PROP_MOT_GAIT_DESC_NAME "MotGaitDesc"

F_DECLARE_INTERFACE(IMotGaitDescProperty);
#undef INTERFACE
#define INTERFACE IMotGaitDescProperty

DECLARE_PROPERTY_INTERFACE(IMotGaitDescProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sMGaitSkillData*);
};


//
// Functions
//

EXTERN void CoreMotionPropsInit();

EXTERN void CoreMotionPropsShutdown();

EXTERN void MotorControllerPropInit(char **motControllerNames,int nMotControllers);

EXTERN BOOL ObjGetMotControllerID(ObjID obj, int *pID);

EXTERN BOOL ObjGetTimeWarp(ObjID obj, float *pWarp);

EXTERN BOOL ObjGetSwordActionType(ObjID obj, int *pType);

EXTERN BOOL ObjGetGaitData(ObjID obj, sMGaitSkillData **pData);

EXTERN BOOL ObjGetActorTagList(ObjID obj, sMotActorTagList **pData);

EXTERN BOOL ObjGetPlayerPosOffsets(ObjID obj, mxs_vector *pPosOff, mxs_angvec *pAngOff);

#endif
