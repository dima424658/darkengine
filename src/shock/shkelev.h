/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkelev.h,v 1.3 2000/01/31 09:55:40 adurant Exp $
#pragma once

#ifndef __SHKELEV_H
#define __SHKELEV_H

#include <rect.h>

EXTERN void ShockElevInit(int which);
EXTERN void ShockElevTerm(void);
EXTERN void ShockElevNetInit();
EXTERN void ShockElevNetTerm();
EXTERN void ShockElevDraw(void);
EXTERN bool ShockElevHandleMouse(Point pt);
EXTERN void ShockElevStateChange(int which);
EXTERN bool ShockElevCheckTransp(Point pt);


#endif
