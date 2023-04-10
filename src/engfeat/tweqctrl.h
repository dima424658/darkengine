/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqctrl.h,v 1.4 2000/01/31 09:45:43 adurant Exp $
// property declaration
#pragma once

#ifndef __TWEQCTRL_H
#define __TWEQCTRL_H

#include <tweqenum.h>

/////////////
// frame update
EXTERN void RunAllTweqs(int ms);

/////////////
// setup/closedown
EXTERN void SetupAllTweqs(void);
EXTERN void TerminateTweqs(void);

// really in twqscrpt
EXTERN void TweqProcessAll(ObjID obj, eTweqDo action);

/////////////
// for setting lock target positions
EXTERN BOOL LockAnimSetTarget(ObjID obj, float where);

/////////////
// get model parameters
EXTERN float *getTweqJointParms(ObjID obj);

#endif  // __TWEQCTRL_H
