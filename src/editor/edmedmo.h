/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/edmedmo.h,v 1.4 2000/01/29 13:11:58 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   edmedmo.h

   export for marking cells in the world with the appropriate medium
   motion indices

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _EDMEDMO_H_
#define _EDMEDMO_H_

#include <lg.h>
#include <portal.h>
#include <texmem.h>
#include <edmedmos.h>

// In which we plow through the world rep and clear the motion_index
// field of every cell.
EXTERN void EdMedMoClearMediumMotionInPortals(void);

// When we start the editor or clear the level, we reset the medium
// motion structures, both inside portal and on our side.
EXTERN void EdMedMoClearMediumMotion(void);

// This is called for each water motion brush ("flow brush"?).
// It sets the medium for all cells which
//  * are flagged by our bounding function;
//  * can be reached from our seed point; and
//  * have the given medium, or one will turn into it (this lets us
//     flood temporary media like door brushes).
//
// We return the number of cells marked.
EXTERN int EdMedMoSetCells(mxs_vector *seed_pos, uchar medium,
                           uchar motion_index,
                           bool(*bounding_function)(mxs_vector *sample_point));

// pop up editing of position and motion for medium indices--
// the return value just tells us whether it worked
EXTERN BOOL EdMedMoSDESC(int motion_index);

EXTERN sEdMedMoSurface g_aEdMedMoSurface[MAX_CELL_MOTION];

// This iterator finds those sEdMedMoSurfaces which have nonempty
// texture_name_base fields.  NULL means there's no more.
EXTERN void EdMedMoSurfaceIterInit(tEdMedMoSurfaceIter *i);
EXTERN sEdMedMoSurface *EdMedMoSurfaceIterNext(tEdMedMoSurfaceIter *i);

// Look up water textures 
EXTERN BOOL GetWaterTexIDs(char *prefix, short *in_hnd, short *out_hnd);



#endif // ~_EDMEDMO_H_
