/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiteams.cpp,v 1.3 2000/02/19 12:45:54 toml Exp $
//
//
//

#include <aiprcore.h>
#include <aiteams.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

EXTERN const char * g_ppszAITeams[kAIT_NumTeams] = 
{
   "Good",                                       // kAIT_Good
   "Neutral",                                    // kAIT_Neutral
   "Bad 1",                                      // kAIT_Bad1
   "Bad 2",                                      // kAIT_Bad2
   "Bad 3",                                      // kAIT_Bad3
   "Bad 4",                                      // kAIT_Bad4
   "Bad 5"                                       // kAIT_Bad5
};

int AITeamCompare(ObjID obj1, ObjID obj2)
{
   return AITeamCompare(AIGetTeam(obj1), AIGetTeam(obj2));
}
