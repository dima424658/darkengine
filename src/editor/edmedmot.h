/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/edmedmot.h,v 1.2 2000/01/29 13:12:00 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   edmedmot.h

   This is the part of the medium motion data which is needed by the
   editor but not by the game.  Well, actually, it's just the typedef.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _EDMEDMOT_H_
#define _EDMEDMOT_H_

// The _change fields are for making the water move: dposition/second.
typedef struct sEdMedMoSurface sEdMedMoSurface;

typedef int tEdMedMoSurfaceIter;

#endif // ~_EDMEDMOT_H_
