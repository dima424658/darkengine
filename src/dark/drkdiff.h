/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkdiff.h,v 1.3 1999/06/02 16:55:05 XEMU Exp $
#pragma once  
#ifndef __DRKDIFF_H
#define __DRKDIFF_H

#include <comtools.h>
#include <objtype.h>
////////////////////////////////////////////////////////////
// DARK DIFFICULTY LEVEL SUPPORT 
//


//
// The quest variable
//
#define DIFF_QVAR "Difficulty" 

//
// Properties
//
#define PROP_DIFF_DESTROY_NAME "DiffDestroy" 
#define PROP_DIFF_PERMIT_NAME "DiffPermit" 
#define PROP_DIFF_LOCK_NAME "DiffLock" 
#define PROP_DIFF_CLOSE_NAME "DiffClose"
#define PROP_DIFF_TURNON_NAME "DiffTurnOn"
#define PROP_DIFF_SCRIPT_NAME "DiffScript"


//
// Init/Term
//
EXTERN void DarkDifficultyInit(void); 
EXTERN void DarkDifficultyTerm(void); 

// Run through the level making difficulty-based modifications 
EXTERN void DarkPrepLevelForDifficulty(void); 

// Is this object allowed at this difficulty level?
EXTERN BOOL DarkDifficultyIsAllowed(ObjID obj, int diff);

#endif // __DRKDIFF_H







