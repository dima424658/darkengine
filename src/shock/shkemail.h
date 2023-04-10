/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkemail.h,v 1.6 2000/01/31 09:55:41 adurant Exp $
#pragma once

#ifndef __SHKEMAIL_H
#define __SHKEMAIL_H

extern "C"
{
#include <event.h>
}

EXTERN void ShockEmailInit(int which);
EXTERN void ShockEmailTerm(void);
EXTERN void ShockEmailDraw(void);
EXTERN bool ShockEmailHandleMouse(Point pt);
EXTERN void ShockEmailStateChange(int which);
EXTERN bool ShockEmailCheckTransp(Point pt);

EXTERN void ShockEmailPlayAudio(char *artname);
EXTERN void ShockEmailStop(void);
EXTERN void ShockEmailDisplay(int mode, int level, int which, BOOL show_mfd);

EXTERN int ShockFindLogData(ObjID obj, int usetype, int *plevel, int *pwhich);
#endif