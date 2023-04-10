/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simtbase.h,v 1.4 2000/01/31 10:00:51 adurant Exp $
#pragma once

#ifndef __SIMTBASE_H
#define __SIMTBASE_H


#include <simtime.h>

#define DEFAULT_MIN_SIM_TIME (SIM_TIME_SECOND/100)
#define DEFAULT_MAX_SIM_TIME (SIM_TIME_SECOND/8)

#define NO_MAX_TIME -1

struct sSimTimingParams 
{
   tSimTime minFrameTime;
   tSimTime maxFrameTime;
   float    scaleFrameTime;
};

#endif // __SIMTBASE_H
