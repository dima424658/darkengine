/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_rend.h,v 1.3 2000/01/29 13:12:12 adurant Exp $
#pragma once

#ifndef __GED_REND_H
#define __GED_REND_H

////////////////////////
// actual brush renderer

// do i show the current selection?
EXTERN BOOL gedrend_show_current;
// render a brush list from camera, flags as "filter"
EXTERN void gedrendRenderView(int flags, int camera);
// these are really taken in the call - most are in brrend
#define GEDREND_FLAG_CURSORONLY (0x10000)  // bottom 16 bits are brFLAG controls


///////////////////////
// camera circle renderer

EXTERN int gedrend_camera_pos_color;
EXTERN void  gedrendRenderCameraLoc (mxs_vector *cam_pos, mxs_angvec *cam_ang, mxs_real scale);

#endif  // __GED_REND_H
