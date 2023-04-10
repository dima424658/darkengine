/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCSECUR_H
#define __DPCSECUR_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCSecurityInit(int which);
EXTERN void DPCSecurityTerm(void);
EXTERN void DPCSecurityDraw(unsigned long inDeltaTicks);
EXTERN bool DPCSecurityHandleMouse(Point pt);
EXTERN void DPCSecurityStateChange(int which);
EXTERN bool DPCSecurityCheckTransp(Point pos);

#endif  // __DPCSECUR_H