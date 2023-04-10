/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinv.h,v 1.5 2000/01/31 09:39:56 adurant Exp $
// dark specific inventory stuff
#pragma once

#ifndef __DRKINV_H
#define __DRKINV_H

#include <drkienum.h>

// initialize the drk inv/ui controller
EXTERN void drkInvInit(void);

// check head focus to see if we have focus on an obj in the world
EXTERN void drkCheckHeadFocus(void);

EXTERN BOOL head_focus_nonfrob;
EXTERN float head_focus_dist2_tol;

// control capabilities and speed associated with inv actions/objects
EXTERN void drkInvCapControl(eDrkInvCap cap, eDrkInvControl control);
EXTERN void drkInvSpeedControlHack(float speed_fac, float rot_fac);

#endif  // __DRKINV_H


