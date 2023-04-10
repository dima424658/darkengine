/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCHPLUG_H
#define __DPCHPLUG_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // ! _OBJTYPE_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCHRMPlugInit(int which);
EXTERN void DPCHRMPlugTerm(void);
EXTERN void DPCHRMPlugDraw(unsigned long inDeltaTicks);
EXTERN void DPCHRMPlugSetMode(int mode, ObjID obj, int bonus = 0);
EXTERN void DPCHRMPlugStateChange(int which);
EXTERN bool DPCHRMPlugCheckTransp(Point pt);

#endif  // __DPCHPLUG_H