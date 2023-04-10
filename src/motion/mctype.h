/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mctype.h,v 1.4 2000/01/31 09:50:10 adurant Exp $
#pragma once

#ifndef __MCTYPE_H
#define __MCTYPE_H

typedef class IMotionCoordinator IMotionCoordinator;
typedef class IManeuver IManeuver;
typedef class IMotionPlan IMotionPlan;

typedef struct sMcMoveState sMcMoveState;
typedef struct sMcMoveParams sMcMoveParams;

typedef void (*fMcMotorStateChangeCallback)(void *context);

typedef void (*fMcManeuverCompletionCallback)(void *context, IManeuver *pCompleted, IManeuver *pNext);

#endif
