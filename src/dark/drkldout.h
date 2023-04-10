/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkldout.h,v 1.1 1998/07/30 23:13:20 mahk Exp $
#pragma once  
#ifndef __LOADOUT_H
#define __LOADOUT_H

////////////////////////////////////////////////////////////
// REMEDIAL LOADOUT UI
//

//
// App Init/Term
//

EXTERN void LoadoutInit(void);
EXTERN void LoadoutTerm(void); 

//
// describe loadout mode, so you can switch to it. 
//
typedef struct sLoopInstantiator sLoopInstantiator; 
EXTERN const struct sLoopInstantiator* DescribeLoadoutMode(void); 

// Just push it to the mode stack already
EXTERN void SwitchToLoadoutMode(BOOL push); 

// This is the quest variable we use for money
#define MONEY_QVAR "TOTAL_LOOT"



#endif // __LOADOUT_H

