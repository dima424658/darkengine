/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCEMAIL_H
#define __DPCEMAIL_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCEmailInit(int which);
EXTERN void DPCEmailTerm(void);
EXTERN void DPCEmailDraw(unsigned long inDeltaTicks);
EXTERN bool DPCEmailHandleMouse(Point pt);
EXTERN void DPCEmailStateChange(int which);
EXTERN bool DPCEmailCheckTransp(Point pt);

EXTERN void DPCEmailPlayAudio(char *artname);
EXTERN void DPCEmailStop(void);
EXTERN void DPCEmailDisplay(int mode, int level, int which, BOOL show_mfd);

EXTERN int DPCFindLogData(ObjID obj, int usetype, int *plevel, int *pwhich);
#endif  // __DPCEMAIL_H