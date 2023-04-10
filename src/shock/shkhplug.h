/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhplug.h,v 1.3 2000/01/31 09:56:39 adurant Exp $
#pragma once

#ifndef __SHKHPLUG_H
#define __SHKHPLUG_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockHRMPlugInit(int which);
EXTERN void ShockHRMPlugTerm(void);
EXTERN void ShockHRMPlugDraw(void);
EXTERN void ShockHRMPlugSetMode(int mode, ObjID obj, int bonus = 0);
EXTERN void ShockHRMPlugStateChange(int which);
EXTERN bool ShockHRMPlugCheckTransp(Point pt);

#endif