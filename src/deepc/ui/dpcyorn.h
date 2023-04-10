/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCYORN_H
#define __DPCYORN_H

extern "C"
{
#include <event.h>
}

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // _OBJTYPE_H

EXTERN void DPCYorNInit(int which);
EXTERN void DPCYorNTerm(void);
EXTERN void DPCYorNDraw(unsigned long inDeltaTicks);
EXTERN bool DPCYorNHandleMouse(uiMouseEvent *mev);
EXTERN void DPCYorNStateChange(int which);
EXTERN bool DPCYorNCheckTransp(Point pt);
EXTERN void DPCYorNPopup(ObjID o, const char *text);

#endif  // __DPCYORN_H