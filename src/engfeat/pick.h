/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/pick.h,v 1.2 2000/01/29 13:19:51 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   pick.h

   This is a fairly generic system for choosing which object the player
   can frob.  The only info it exports is g_PickCurrentObj, which is
   updated every frame, and equals OBJ_NULL if there's nothing in front
   of the player camera to be picked.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#ifndef __PICK_H
#define __PICK_H

#include <lg.h>
#include <fix.h>

// This describes a bounding box in camera space.  The first two
// parameters are in screen space, and the third in world space,
// cause hey, it's convenient.
EXTERN void PickInit();
EXTERN void PickTerm();
EXTERN void PickSetFocus(fix FocusX, fix FocusY, float fMaxDistSquared);
EXTERN void PickFrameUpdate(void);
EXTERN void PickSetCanvas(void);
EXTERN ObjID PickWeighObject(ObjID obj);

EXTERN ObjID g_PickCurrentObj;

#endif // __PICK_H
