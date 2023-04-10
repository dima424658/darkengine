/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpda.h,v 1.8 2000/01/31 09:58:23 adurant Exp $
#pragma once

#ifndef __SHKPDA_H
#define __SHKPDA_H

extern "C"
{
#include <event.h>
}

EXTERN void ShockPDAInit(int which);
EXTERN void ShockPDATerm(void);
EXTERN void ShockPDADraw(void);
EXTERN bool ShockPDAHandleMouse(Point pt);
EXTERN void ShockPDAStateChange(int which);
EXTERN BOOL ShockPDANeedLog(int usetype, int uselevel, int usewhich);
EXTERN void ShockPDAUseLog(int usetype, int uselevel, int usewhich, BOOL show_mfd);
EXTERN bool ShockPDACheckTransp(Point pos);

EXTERN void ShockPDASetTime(int uselevel, int usetype, int which);
EXTERN void ShockPDASetFilter(int which);

EXTERN void ShockPDANetInit(void);
EXTERN void ShockPDANetTerm(void);

#endif
