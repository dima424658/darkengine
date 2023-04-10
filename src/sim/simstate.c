/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simstate.c,v 1.3 2000/02/19 13:27:47 toml Exp $

#include <simstate.h>
#include <simbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static SimState  DefaultState; 
static SimState* TheSimState = &DefaultState;

static SimState* SaveSimState = NULL;
static SimState PausedState; // Sim state we use when pausing the sim
static SimState PausedValues = { 0 }; // intial values for the "paused" state

static ulong PauseCount = 0;

////////////////////////////////////////

void SimStateSet(SimState* state)
{
   if (state == NULL)
      TheSimState = &DefaultState;
   else
      TheSimState = state;
}

////////////////////////////////////////

SimState* SimStateGet(void)
{
   return TheSimState;
}

////////////////////////////////////////

SimFlags SimStateCheckFlags(SimFlags flags)
{
   return TheSimState->flags & flags;
}

////////////////////////////////////////

void SimStateSetFlags(SimFlags flags, BOOL set)
{
   if (set)
      TheSimState->flags |= flags;
   else
      TheSimState->flags &=~flags; 
}

////////////////////////////////////////

EXTERN void SimStatePause()
{
   PauseCount++;
   if (PauseCount == 1)
   {
      // copy initial values into paused sim state
      PausedState = PausedValues;
      // Save the sim state
      SaveSimState = TheSimState;
      TheSimState = &PausedState;
   }

}

EXTERN void SimStateUnpause()
{
   PauseCount++; // Lock out other people trying to unpause at the same time
   if (PauseCount == 2)
   {
      if (TheSimState == &PausedState && SaveSimState != NULL) 
         TheSimState = SaveSimState;
   }
   PauseCount -= 2;
}







