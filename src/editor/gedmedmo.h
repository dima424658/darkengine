/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gedmedmo.h,v 1.3 2000/01/29 13:12:07 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   gedmedmo.h

   export for gedmedmo.c

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _GEDMEDMO_H_
#define _GEDMEDMO_H_

// We use this when only one brush has changed.  We want to call it
// first with the brush's previous geometry and medium index 0 to
// clear things out.  Then with the new geometry and real index.
EXTERN void GEdMedMoMarkWaterOneBrush(editBrush *brush, mxs_vector *seed);

// When we want set the motion for every cell in the world rep, using
// all the brushes which do that sort of thing, we do this.
EXTERN void GEdMedMoMarkWaterAllBrushes(void);

#endif // ~ _GEDMEDMO_H_
