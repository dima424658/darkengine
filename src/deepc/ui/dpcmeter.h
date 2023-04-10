/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCMETER_H
#define __DPCMETER_H

extern "C" 
{
#include <event.h>
}

EXTERN void DPCMetersDraw(unsigned long inDeltaTicks);
EXTERN void DPCMetersInit(int which);
EXTERN void DPCMetersTerm(void);
EXTERN bool DPCMetersHandleMouse(Point pt);
EXTERN bool DPCMetersCheckTransp(Point pt);
EXTERN bool DPCMetersDragDrop(Point pt, BOOL start);

#endif  // __DPCMETER_H
