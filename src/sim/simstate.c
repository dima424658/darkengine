/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simstate.c,v 1.3 2000/02/19 13:27:47 toml Exp $

#include <simstate.h>
#include <simbase.h>
#ifdef __ANDROID__
#include <stdio.h>
#include <simtime.h>
#include <platform_services.h>
#endif
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static SimState  DefaultState; 
static SimState* TheSimState = &DefaultState;

static SimState* SaveSimState = NULL;
static SimState PausedState; // Sim state we use when pausing the sim
static SimState PausedValues = { 0 }; // intial values for the "paused" state

static ulong PauseCount = 0;

#ifdef __ANDROID__
static void trace_sim_state(const char* action)
{
   char buffer[192];
   const SimFlags flags = TheSimState ? TheSimState->flags : 0;
   const SimFlags savedFlags = SaveSimState ? SaveSimState->flags : 0;
   snprintf(buffer, sizeof(buffer),
            "[sim-pause] %s depth=%u flags=0x%08x saved=0x%08x time=%u\n",
            action, (unsigned)PauseCount, (unsigned)flags,
            (unsigned)savedFlags, (unsigned)GetSimTime());
   PlatformDebugOutput(buffer);
}
#else
#define trace_sim_state(action) ((void)0)
#endif

////////////////////////////////////////

void SimStateSet(SimState* state)
{
   if (state == NULL)
      TheSimState = &DefaultState;
   else
      TheSimState = state;
   trace_sim_state("set");
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
   }

   // Mode transitions are allowed to install their own SimState while a
   // panel is suspended.  A later nested panel pause must therefore
   // reassert the paused state even though the outer pause count is still
   // active.  Only the outermost pause owns SaveSimState.
   TheSimState = &PausedState;
   trace_sim_state("pause");

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
   trace_sim_state("unpause");
}







