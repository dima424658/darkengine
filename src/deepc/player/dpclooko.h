/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCLOOK_H
#define __DPCLOOK_H

extern "C"
{
#include <event.h>
}
#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

EXTERN void DPCLookInit(int which);
EXTERN void DPCLookTerm(void);
EXTERN void DPCLookDraw(unsigned long inDeltaTicks);
EXTERN bool DPCLookHandleMouse(Point pt);
EXTERN void DPCLookStateChange(int which);
EXTERN bool DPCLookCheckTransp(Point pt);
EXTERN void DPCLookPopup(ObjID o); // , const char *text);
EXTERN void DPCLookCursor(void);

#endif  // __DPCLOOK_H