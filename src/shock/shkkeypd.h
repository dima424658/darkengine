/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkkeypd.h,v 1.4 2000/01/31 09:57:46 adurant Exp $
#pragma once

#ifndef __SHKKEYPD_H
#define __SHKKEYPD_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockKeypadInit(int which);
EXTERN void ShockKeypadTerm(void);
EXTERN void ShockKeypadDraw(void);
EXTERN bool ShockKeypadHandleMouse(Point pt);
EXTERN bool ShockKeypadHandleKey(int keycode);
EXTERN void ShockKeypadStateChange(int which);
EXTERN void ShockKeypadOpen(ObjID o);
EXTERN bool ShockKeypadCheckTransp(Point pt);


#endif