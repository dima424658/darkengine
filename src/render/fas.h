/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/fas.h,v 1.2 1998/09/14 00:50:06 dc Exp $

#pragma once

#ifndef __FAS_H
#define __FAS_H

#include <lg.h>

// these are fully defined in fastype.h
typedef struct FrameAnimationState FrameAnimationState;
typedef struct FrameAnimationConfig FrameAnimationConfig;

// compute and return the current of the animation
EXTERN int FrameAnimationGetFrame(FrameAnimationState *state,
                                  FrameAnimationConfig *cfg, int num_frames);

// just tell me if it is done
EXTERN BOOL FrameAnimationIsDone(FrameAnimationState *fas);

// configure a frame animation to start right now
EXTERN void FrameAnimationStateStart(FrameAnimationState *);

// configure a frame animation to start in the middle right now
EXTERN void FrameAnimationStateSetFrame(FrameAnimationState *,
                                        FrameAnimationConfig *, int cur_frame);

// configure a frame animation to start at some known time
EXTERN void FrameAnimationStateBackdateStart(FrameAnimationState *,
                             FrameAnimationConfig *, ulong tm);
EXTERN void FrameAnimationStateBackdateSetFrame(FrameAnimationState *,
                             FrameAnimationConfig *, int frame, ulong tm);

// if you want to change the animation without restarting it, call this:
EXTERN void FrameAnimationChange(FrameAnimationState  *state,
                             FrameAnimationConfig *old_cfg, int old_frames,
                             FrameAnimationConfig *new_cfg, int new_frames);

#endif
