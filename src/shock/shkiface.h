/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkiface.h,v 1.12 2000/01/31 09:56:42 adurant Exp $
#pragma once

#ifndef __SHKIFACE_H
#define __SHKIFACE_H

#include <objtype.h>

extern "C" 
{
#include <event.h>
}
   
// Draw the given string in the given color, with the cursor before
// character cursor. If cursor == -1, then no cursor is drawn:
EXTERN void ShockInterfaceDrawString(char *str, int color, int cursor);
EXTERN void ShockInterfaceDraw(void);
EXTERN void ShockInterfaceInit(int which);
EXTERN void ShockInterfaceTerm(void);
EXTERN bool ShockInterfaceHandleMouse(Point pos);
EXTERN bool ShockInterfaceCheckTransp(Point pt);
EXTERN void ShockInterfaceStateChange(int which);
EXTERN void ShockInterfaceMouseOver(ObjID o);

EXTERN void ShockMiniFrameDraw(void);
EXTERN void ShockMiniFrameInit(int which);
EXTERN void ShockMiniFrameTerm(void);

EXTERN ObjID g_ifaceFocusObj;
EXTERN BOOL gFocusTouched;
#endif
