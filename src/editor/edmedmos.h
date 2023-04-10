/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/edmedmos.h,v 1.4 2000/01/29 13:11:59 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   edmedmos.h

   This is the part of the medium motion data which is needed by the
   editor but not by the game.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _EDMEDMOS_H_
#define _EDMEDMOS_H_

#include <edmedmot.h>
#include <lress.h>

#define kMedmoSurfaceFilenameLength (RN_NAME_LEN + 1)

// The _change fields are for making the water move: dposition/second.
struct sEdMedMoSurface {
   short texture_above;
   short texture_below;
   char texture_name_base[kMedmoSurfaceFilenameLength];
   char pad[12];   // pad, used to be rgb stuff 
};

#endif // ~_EDMEDMOS_H_
