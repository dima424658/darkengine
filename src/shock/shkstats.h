/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkstats.h,v 1.3 2000/01/31 09:59:18 adurant Exp $
#pragma once

#ifndef __SHKSTATS_H
#define __SHKSTATS_H

extern "C"
{
#include <event.h>
}

EXTERN void ShockStatsInit(int which);
EXTERN void ShockStatsTerm(void);
EXTERN void ShockStatsDraw(void);
EXTERN bool ShockStatsHandleMouse(Point pt);
EXTERN void ShockStatsStateChange(int which);
EXTERN bool ShockStatsCheckTransp(Point pt);


#endif