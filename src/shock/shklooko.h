/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklooko.h,v 1.3 2000/01/31 09:57:50 adurant Exp $
#pragma once

#ifndef __SHKLOOK_H
#define __SHKLOOK_H

extern "C"
{
#include <event.h>
}
#include <objtype.h>

EXTERN void ShockLookInit(int which);
EXTERN void ShockLookTerm(void);
EXTERN void ShockLookDraw(void);
EXTERN bool ShockLookHandleMouse(Point pt);
EXTERN void ShockLookStateChange(int which);
EXTERN bool ShockLookCheckTransp(Point pt);
EXTERN void ShockLookPopup(ObjID o); // , const char *text);
EXTERN void ShockLookCursor(void);

#endif