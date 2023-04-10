/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmislp.h,v 1.7 1999/10/11 16:21:38 adurant Exp $
#pragma once  
#ifndef __DRKMISLP_H
#define __DRKMISLP_H

////////////////////////////////////////////////////////////
// DARK MISSION LOOP
//

EXTERN void MissionLoopInit(void); 
EXTERN void MissionLoopTerm(void); 

EXTERN struct sLoopInstantiator* DescribeMissionLoopMode(void); 

//
// Unwind to the mission loop mode 
//
EXTERN void UnwindToMissionLoop(void); 

//
// Reset the mission loop to an appropriate start state
//

enum eMissLoopReset
{
   kMissLoopMainMenu,      // main menu
   kMissLoopStartLoop,     // Start mission loop
   kMissLoopRestartMission, // Restart mission 
}; 

EXTERN void MissionLoopReset(int to_where); 

//
// Set/Get which mission is next
//
EXTERN void SetNextMission(int mission); 
EXTERN int GetNextMission(void); 

//
// Show the briefing movie
//
EXTERN void PushBriefingMovieMode(int mission); 

//
// Show a movie, or a book if you can't find it 
// pass in an fname with no suffix.
// It will use foo.avi for the movie, foo.str for the book.  
// the config var foo_text_bg for the book art fname
//
EXTERN void PushMovieOrBookMode(const char* fname); 

//
// Is difficulty frozen
//
EXTERN BOOL CanChangeDifficultyNow(void); 

#endif // __DRKMISLP_H

