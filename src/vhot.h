/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/vhot.h,v 1.6 2000/01/29 12:41:52 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   vhot.h

   Each kind of vhot for md models has a ulong id.  You have reached
   the inner sanctum of vhot identification.  Convert now to the one
   true cause or suffer like a heathen pig wallowing in molten swill.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _VHOT_H_
#define _VHOT_H_

#define kVHotLightSource  1     // for example, the flame on a torch
#define kVHotAnchor       2     // To anchor 3d model frames if tweq set

#define kVHotParticle1    3     // for use in launch points for particles
#define kVHotParticle2    4     // for use in launch points for particles
#define kVHotParticle3    5     // for use in launch points for particles
#define kVHotParticle4    6     // for use in launch points for particles
#define kVHotParticle5    7     // for use in launch points for particles
#define kVHotLightSource2 8     // Provide spotlight direction info relative to kVHotLightSource


#endif // ~_VHOT_H_
