/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCROSS_H
#define __DPCCROSS_H

#ifndef RECT_H
#include <rect.h>
#endif // !RECT_H

EXTERN void DPCCrosshairDraw(unsigned long inDeltaTicks);
EXTERN void DPCCrosshairInit(int which);
EXTERN void DPCCrosshairTerm(void);
EXTERN bool DPCCrosshairCheckTransp(Point pt);

#endif  // !__DPCCROSS_H