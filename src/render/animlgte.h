/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/animlgte.h,v 1.4 2000/01/29 13:38:25 adurant Exp $
// Anim light enums
// Put into own file for use in scripts
#pragma once

#define ANIM_LIGHT_MODE_FLIP 0
// slides between extreme states
#define ANIM_LIGHT_MODE_SMOOTH 1
// ignores timing and picks intensity at random from given range
#define ANIM_LIGHT_MODE_RANDOM 2
// stuck on minimum intensity
#define ANIM_LIGHT_MODE_MINIMUM 3
// stuck on maximum intensity
#define ANIM_LIGHT_MODE_MAXIMUM 4
// stuck at the end of the world
#define ANIM_LIGHT_MODE_EXTINGUISH 5
// smoothly transit from min to max, then stop
#define ANIM_LIGHT_MODE_SMOOTH_BRIGHTEN   6
// smoothly transit from max to min, then stop
#define ANIM_LIGHT_MODE_SMOOTH_DIM  7
// like random, but with some coherence (limited changer per frame)
#define ANIM_LIGHT_MODE_RAND_COHERENT  8
// flickers from min to max randomly
#define ANIM_LIGHT_MODE_FLICKER  9
