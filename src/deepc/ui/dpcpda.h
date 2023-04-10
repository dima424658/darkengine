/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPDA_H
#define __DPCPDA_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCPDAInit(int which);
EXTERN void DPCPDATerm(void);
EXTERN void DPCPDADraw(unsigned long inDeltaTicks);
EXTERN bool DPCPDAHandleMouse(Point pt);
EXTERN void DPCPDAStateChange(int which);
EXTERN BOOL DPCPDANeedLog(int usetype, int uselevel, int usewhich);
EXTERN void DPCPDAUseLog(int usetype, int uselevel, int usewhich, BOOL show_mfd);
EXTERN bool DPCPDACheckTransp(Point pos);

EXTERN void DPCPDASetTime(int uselevel, int usetype, int which);
EXTERN void DPCPDASetFilter(int which);

EXTERN void DPCPDANetInit(void);
EXTERN void DPCPDANetTerm(void);

#endif  // __DPCPDA_H
