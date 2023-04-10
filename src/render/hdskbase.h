/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/hdskbase.h,v 1.2 2000/01/29 13:38:58 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   hdskbase.h

   heat disk special effect property

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _HDSKBASE_H_
#define _HDSKBASE_H_

#include <matrixs.h>

// This is a string of little blobby heat effects, all of which are
// little vertical cones.
typedef struct sHeatDiskCluster {
   // These are offsets from the center of the object, in the object's
   // coordinate system.  We put them into world space every frame.
   mxs_vector start_offset, end_offset;

   // These are the radii of the heat disks at the ends, in world
   // space.
   mxs_real start_radius, end_radius;

   // These are also in world space.  They are used to scale the
   // random offsets at which the inner and outer disks are placed
   // every frame, so they can jiggle.
   mxs_real bottom_jitter, top_jitter;

   // How tall is each little blob?
   mxs_real height;

   // how many blobs can one cluster have,
   // before you can call it a cluster?
   int num_effects;

   // disks/blob at z=1
   int num_disks;
} sHeatDiskCluster;


#endif  // ~_HDSKBASE_H_
