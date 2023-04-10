/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/medmotn.h,v 1.4 2000/01/29 13:39:06 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   medmotn.h

   interface into medium motion module, in which we move water
   surfaces by changing u, v offsets and vectors stored in Portal and
   select and clear associated structures in the editor

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _MEDMOTN_H_
#define _MEDMOTN_H_

#include <matrix.h>

#include <portal.h>
#include <wr.h>

#include <medmotns.h>


// We'll install this as the callback for portal_anim_medium_callback().
EXTERN void MedMoUpdateCellMotion(long frame_time_ms);

// Each of these corresponds to one element of portal_cell_motion[].
EXTERN sMedMoCellMotion g_aMedMoCellMotion[MAX_CELL_MOTION];

#endif
