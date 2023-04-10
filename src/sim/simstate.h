/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simstate.h,v 1.3 2000/01/31 10:00:50 adurant Exp $
#pragma once

#ifndef SIMSTATE_H
#define SIMSTATE_H
#include <lg.h>

////////////////////////////////////////////////////////////
// SIMULATION STATE API
//
// The simulation state is a collection of global flags and other
// stuff that wants to be swapped out wholesale on mode changes.
//

typedef struct SimState SimState; 
typedef ulong SimFlags; 

//
// Set the current simulation state.  Does not copy, but rather changes 
// the pointer to the global sim state.
//

EXTERN void SimStateSet(SimState* state); 

//
// Get the current sim state.
//
EXTERN SimState* SimStateGet(void);

//
// Access the flags in the current sim state
//
EXTERN SimFlags SimStateCheckFlags(SimFlags flags); // returns the flags that are true. 
EXTERN void SimStateSetFlags(SimFlags flags,BOOL set); // sets or clears flags

//
// Pauses and unpauses the sim.  stackable.  
//
EXTERN void SimStatePause(void);
EXTERN void SimStateUnpause(void);


#endif // SIMSTATE_H
