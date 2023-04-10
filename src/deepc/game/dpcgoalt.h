/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/deepc/game/dpcgoalt.h,v 1.1 2000/01/26 19:07:05 porges Exp $
#pragma once  
#ifndef __DPCGOALT_H
#define __DPCGOALT_H

//
// This file can be used from a script as well as normal code

#ifdef SCRIPT
#include <scrpttyp.h>
typedef cScrStr cVarName;
#else 
#include <str.h>
typedef cStr cVarName; 
#endif 

////////////////////////////////////////////////////////////
// TYPES FOR GOALS (MISSION OBJECTIVES) IN DARK 
//

//
// Goals are numbered starting with zero.  Each goal is in one of 3 states. 
//

enum eGoalState
{
   kGoalIncomplete, // You haven't completed this goal 
   kGoalComplete,   // You've completed this goal 
   kGoalInactive,   // You don't need to complete this goal
   kGoalFailed,     // You already failed this goal 
}; 

//
// Goal states are stored in quest variables.  The number of goals in the mission is 
// determined by the number of these quest variables that are defined.  In an N-objective 
// mission the first N state variables must be defined in the mission, and only those. 
//

// Return the name of the goal state variable for goal #i 
inline cVarName GoalStateVarName(int i)
{
   char buf[64]; 
   sprintf(buf,"GOAL_STATE_%d",i); 
   return buf; 
}

//
// We also store whether a goal is visible to the UI in a quest variable 
//

// Return the name of the goal visibility variable for goal #i 
inline cVarName GoalVisibleVarName(int i)
{
   char buf[64]; 
   sprintf(buf,"GOAL_VISIBLE_%d",i); 
   return buf; 
}; 

//
// Optional "min difficulty" and "max difficulty" qvars 
//

inline cVarName GoalMinDiffVarName(int i)
{
   char buf[64]; 
   sprintf(buf,"GOAL_MIN_DIFF_%d",i); 
   return buf; 
}; 

inline cVarName GoalMaxDiffVarName(int i)
{
   char buf[64]; 
   sprintf(buf,"GOAL_MAX_DIFF_%d",i); 
   return buf; 
}; 

#define MISSION_COMPLETE_VAR "MISSION_COMPLETE"

#endif // __DPCGOALT_H




