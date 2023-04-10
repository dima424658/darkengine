/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simtime.h,v 1.4 2000/01/31 10:00:52 adurant Exp $
#pragma once

#ifndef __SIMTIME_H
#define __SIMTIME_H

typedef ulong tSimTime;
#define SIM_TIME_SECOND 1000 // one second in our units
#define SIM_TIME_MINUTE (SIM_TIME_SECOND*60)
#define SIM_TIME_HOUR   (SIM_TIME_MINUTE*60)


typedef struct sSimTimingParams sSimTimingParams;

////////////////////////////////////////////////////////////
// SIM TIME API
// 
// Sim time is the measure of time within the simulation.
// It stops when the sim is paused, and is saved with the file.
//


//
// Get the time since the start of the sim.
//

EXTERN tSimTime GetSimTime(void); 

//
// Get the amount of simulation time that passed this frame
//

EXTERN tSimTime GetSimFrameTime(void);

//
// Set the time since sim start.  Some clients may react strangely if this
// does not occur at a database change.
//

EXTERN void SetSimTime(tSimTime time);

//
// Get the the sim time at which the last save/load of this database occurred
//

EXTERN tSimTime GetSimFileTime(void); 

//
// Start and stop the passing of time.
//

EXTERN void SetSimTimePassing(BOOL passing);
EXTERN BOOL IsSimTimePassing(void);


//
// Set the timing parameters, for frame rate governing and probably other stuff.
//

EXTERN void SetSimTimingParams(const sSimTimingParams* params);

EXTERN const sSimTimingParams* GetSimTimingParams(void);

//
// Set the scaling factor at which time passes
//
EXTERN void SetSimTimeScale(float scale);

#endif // __SIMTIME_H






