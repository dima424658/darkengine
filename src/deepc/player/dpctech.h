/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCTECH_H
#define __DPCTECH_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCHackingInit(int which);
EXTERN void DPCHackingTerm(void);
EXTERN void DPCHackingDraw(void);
EXTERN bool DPCHackingHandleMouse(uiMouseEvent *mev);
EXTERN void DPCHackingStateChange(int which);
EXTERN void DPCHackingBegin(ObjID o);
EXTERN bool DPCHackingCheckTransp(Point pt);

#endif  // __DPCTECH_H