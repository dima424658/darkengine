/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/metagame.h,v 1.3 1998/08/11 14:16:12 mahk Exp $
#pragma once  
#ifndef __METAGAME_H
#define __METAGAME_H

////////////////////////////////////////////////////////////
// REMEDIAL METAGAME UI
//

//
// App Init/Term
//

EXTERN void MetaGameInit(void);
EXTERN void MetaGameTerm(void); 

//
// describe metagame mode, so you can switch to it. 
//
typedef struct sLoopInstantiator sLoopInstantiator; 
EXTERN const struct sLoopInstantiator* DescribeMetaGameMode(void); 

// Just push it to the mode stack already
EXTERN void SwitchToMetaGameMode(BOOL push); 


#endif // __METAGAME_H
