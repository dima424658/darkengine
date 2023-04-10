/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/sprks.h,v 1.2 2000/01/31 09:53:28 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   sprks.h

   little triangular spark special effect property

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _SPRKS_H_
#define _SPRKS_H_

#include <matrixs.h>

// This is one little triangle dingus.
struct sSpark {
   uint flags;
   ushort color;                // not used if there's a texmap (duh)
   mxs_ang angle_jitter;        // the angle is tx in our object's Position
   mxs_angvec angle_offset;
   ushort pad;

   mxs_real size[3];
   mxs_real size_jitter;        // ratio: .25 would give us size x [.75, 1.25]

   mxs_real base_light_level;
   mxs_real light_jitter;       // ratio, as with size
};

// lighting flags
#define kSprkFlagLightByVertex      (1 << 0)
#define kSprkFlagBrightHighAngle    (1 << 1)
#define kSprkFlagBrightLowAngle     (1 << 2)

// These two use the x component of the scale prop, and expect [0..1].
#define kSprkFlagBrightHighScale    (1 << 3)
#define kSprkFlagBrightLowScale     (1 << 4)

// texture flags
#define kSprkFlagTextured           (1 << 5)

#endif  // ~_SPRKS_
