/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmeter.h,v 1.4 2000/01/31 09:57:59 adurant Exp $
#pragma once

#ifndef __SHKMETER_H
#define __SHKMETER_H

extern "C" 
{
#include <event.h>
}

EXTERN void ShockMetersDraw(void);
EXTERN void ShockMetersInit(int which);
EXTERN void ShockMetersTerm(void);
EXTERN bool ShockMetersHandleMouse(Point pt);
EXTERN bool ShockMetersCheckTransp(Point pt);
EXTERN bool ShockMetersDragDrop(Point pt, BOOL start);

#endif
