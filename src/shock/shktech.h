/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktech.h,v 1.3 2000/01/31 09:59:22 adurant Exp $
#pragma once

#ifndef __SHKTECH_H
#define __SHKTECH_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockHackingInit(int which);
EXTERN void ShockHackingTerm(void);
EXTERN void ShockHackingDraw(void);
EXTERN bool ShockHackingHandleMouse(uiMouseEvent *mev);
EXTERN void ShockHackingStateChange(int which);
EXTERN void ShockHackingBegin(ObjID o);
EXTERN bool ShockHackingCheckTransp(Point pt);

#endif