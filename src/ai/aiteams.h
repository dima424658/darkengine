/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiteams.h,v 1.3 1998/09/09 13:06:12 TOML Exp $
//
//

#ifndef __AITEAMS_H
#define __AITEAMS_H

#pragma once

///////////////////////////////////////////////////////////////////////////////

enum eAITeam
{
   kAIT_Good,
   kAIT_Neutral,
   kAIT_Bad1,
   kAIT_Bad2,
   kAIT_Bad3,
   kAIT_Bad4,
   kAIT_Bad5,
   
   kAIT_NumTeams,
   
   kAIT_IntMax = 0xffffffff,
};

///////////////////////////////////////

EXTERN const char * g_ppszAITeams[kAIT_NumTeams];

///////////////////////////////////////

#define AIGetTeamName(team) (g_ppszAITeams[(team)])

///////////////////////////////////////

enum eAITeamCompareResult
{
   kAI_Opponents,
   kAI_Teammates,
   kAI_Indifferent
};

inline int AITeamCompare(eAITeam team1, eAITeam team2)
{
   return (team1 == kAIT_Neutral || team2 == kAIT_Neutral) ? 
               kAI_Indifferent : 
               team1 == team2;
}

int AITeamCompare(ObjID obj1, ObjID obj2);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AITEAMS_H */
