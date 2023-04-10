/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkdebrf.h,v 1.3 1998/11/03 06:37:57 mahk Exp $
#pragma once  
#ifndef __DEBRIEF_H
#define __DEBRIEF_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// REMEDIAL DEBRIEF UI
//

//
// App Init/Term
//

EXTERN void DebriefInit(void);
EXTERN void DebriefTerm(void); 

//
// describe debrief mode, so you can switch to it. 
//
typedef struct sLoopInstantiator sLoopInstantiator; 
EXTERN const struct sLoopInstantiator* DescribeDebriefMode(void); 

// Just push it to the mode stack already
EXTERN void SwitchToDebriefMode(BOOL push); 

//
// Objectives screen mode too 
// 

EXTERN const struct sLoopInstantiator* DescribeObjectivesMode(void); 
EXTERN void SwitchToObjectivesMode(BOOL push); 

//
// This is a "loading" screen that actually load the gNextMission, and 
// also displays the objectives 
// 

EXTERN const struct sLoopInstantiator* DescribeLoadingMode(void); 
EXTERN void SwitchToLoadingMode(BOOL push); 
F_DECLARE_INTERFACE(ITagFile); 
EXTERN void PushToSaveGameLoadingMode(ITagFile* file); 



#endif // __DEBRIEF_H

