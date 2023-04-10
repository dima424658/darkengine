/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCKEYPD_H
#define __DPCKEYPD_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // _OBJTYPE_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCKeypadInit(int which);
EXTERN void DPCKeypadTerm(void);
EXTERN void DPCKeypadDraw(unsigned long inDeltaTicks);
EXTERN bool DPCKeypadHandleMouse(Point pt);
EXTERN bool DPCKeypadHandleKey(int keycode);
EXTERN void DPCKeypadStateChange(int which);
EXTERN void DPCKeypadOpen(ObjID o);
EXTERN bool DPCKeypadCheckTransp(Point pt);


#endif // __DPCKEYPD_H