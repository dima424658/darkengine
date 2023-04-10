/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCELEV_H
#define __DPCELEV_H

#ifndef RECT_H
#include <rect.h>
#endif // !RECT_H

EXTERN void DPCElevInit(int which);
EXTERN void DPCElevTerm(void);
EXTERN void DPCElevNetInit();
EXTERN void DPCElevNetTerm();
EXTERN void DPCElevDraw(unsigned long inDeltaTicks);
EXTERN bool DPCElevHandleMouse(Point pt);
EXTERN void DPCElevStateChange(int which);
EXTERN bool DPCElevCheckTransp(Point pt);


#endif
