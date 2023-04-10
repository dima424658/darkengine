/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCIFACE_H
#define __DPCIFACE_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

extern "C" 
{
#include <event.h>
}
   
// Draw the given string in the given color, with the cursor before
// character cursor. If cursor == -1, then no cursor is drawn.
EXTERN void DPCInterfaceDrawString(char *str, int color, int cursor);
EXTERN void DPCInterfaceDraw(unsigned long inDeltaTicks);
EXTERN void DPCInterfaceInit(int which);
EXTERN void DPCInterfaceTerm(void);
EXTERN bool DPCInterfaceHandleMouse(Point pos);
EXTERN bool DPCInterfaceCheckTransp(Point pt);
EXTERN void DPCInterfaceStateChange(int which);
EXTERN void DPCInterfaceMouseOver(ObjID o);

EXTERN void DPCMiniFrameDraw(unsigned long inDeltaTicks);
EXTERN void DPCMiniFrameInit(int which);
EXTERN void DPCMiniFrameTerm(void);

EXTERN ObjID g_ifaceFocusObj;
EXTERN BOOL gFocusTouched;
#endif  // _DPCIFACE_H
