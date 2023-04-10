/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mschbase.h,v 1.8 2000/01/31 09:51:07 adurant Exp $
#pragma once

#ifndef __MSCHBASE_H
#define __MSCHBASE_H

#include <mschtype.h>

#define MAX_GAIT_TURNS 3

typedef struct sMGaitTurn
{
   int angle; // turn angle XXX can this be fixang yet edited in editor as degrees?
   int mot;
} sMGaitTurn;

struct sMGaitSkillData
{
   int leftStride;
   int rightStride;
   float maxAscend; 
   float maxDescend;
   float timeWarp; // slows down playback of all motions for this gait
   float stretch; // horizontally stretches all motions for this gait
   int maxAngVel;  // in degrees/second, used for smoothing
   int turnTolerance; // in degrees.  how close turn angle need to be to choose to turn
   float maxVel; // max "physically possible" velocity appropriate for gait
   float minVel;
   int numTurns;
   sMGaitTurn turns[MAX_GAIT_TURNS];
   float noise;
};

struct sMPlayerSkillData
{
   float      flexDuration; // time to max flex
   int        flexDegree; // maximum flex, in degrees
};

// for mvrcmbat motor controller
enum eMSwordActionType
{
   kMSAT_Swing,
   kMSAT_GeneralBlock,
   kMSAT_DirectedBlock,
   kMSAT_Invalid=0xffffffff,
};

#endif // __MSCHDATA_H
