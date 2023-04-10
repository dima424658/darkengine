/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/animlgt.h,v 1.16 2000/01/29 13:38:24 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   animlgt.h

   animated static lights (that is, animated but w/fixed positions)

   exported functions for animlgt.c

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _ANIMLGT_H_
#define _ANIMLGT_H_


#include <objlight.h>
#include <animlgts.h>

// clear our array positions
EXTERN void AnimLightClear(void);

// Call this once when the program's just starting--it allocates
// portal_anim_light_intensity.
EXTERN void AnimLightInit(void);

// This is the thing to call as we're shutting down our system.
EXTERN void AnimLightCleanup(void);

// When a cell's light level has changed, we call this to force the
// surfaces it reaches to be rebuilt.
//zb EXTERN void AnimLightFlagCells(sAnimLight *light);ObjID obj
EXTERN void AnimLightFlagCells(sAnimLight *light, ObjID obj);//zb

// After we shine an animated light in the world we use this to set
// the list of cells it reached.
EXTERN void AnimLightSetCellList(sAnimLight *light, uint light_data_index);

// We advance a light's timer, in part, to see whether its intensity
// has changed; if it has then we flag it for surface rebuilding.
// The return value indicates whether the timing reversed direction.
EXTERN BOOL AnimLightUpdateTimer(sAnimLight *light, long time_change);


#define MAX_ANIM_LIGHTS MAX_STATIC

// The structures mapping lights to cells are more or less internal,
// but we still gotta save 'em and load 'em, so we still gotta expose
// 'em.  Let's not refer to these all over the place, okay?
//#define MAX_ANIM_LIGHT_TO_CELLS 8192
// I had to double this data structure so that in non-optimized
// levels (where cell counts are pretty high) things would still
// come out right
#define MAX_ANIM_LIGHT_TO_CELLS 16384

EXTERN int g_iCurAnimLightToCell;
EXTERN sAnimLightToCell g_aAnimLightToCell[MAX_ANIM_LIGHT_TO_CELLS];

#endif // ~_ANIMLGT_H_
