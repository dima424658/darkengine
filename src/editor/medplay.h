/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __MEDPLAY_H
#define __MEDPLAY_H

#include <motion.h>

#define PLAY_STRAIGHT      0
#define PLAY_LOOP          1

#define PLAY_MAIN_ONLY     0
#define PLAY_ALL_MOTIONS   1

#define PLAY_NO_BLEND      0
#define PLAY_NORMAL_BLEND  1

#define PLAY_TIME_BASED    0
#define PLAY_FRAME_BASED   1

typedef struct PlayOptions
{
   float trans_duration;
   int trans_offset;
   int loop;
   int which_motions;
   int blend;
   int frame_based;
   float timewarp;
   float stretch;
} PlayOptions;

EXTERN PlayOptions playOptions;

EXTERN mps_motion_param playParams;

EXTERN void PlayOptInit();
EXTERN void PlayOptApply();
EXTERN BOOL PlayMotionStart();
EXTERN void PlayMotionStop();
EXTERN void PlayMotionUpdate(float dt);
EXTERN void PlayMotionPoseAtFrame(int frame);

#endif
