/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklding.h,v 1.4 2000/01/31 09:57:48 adurant Exp $
#pragma once

#ifndef __SHKLDING_H
#define __SHKLDING_H

////////////////////////////////////////////////////////////
// SHOCK LOADING SCREEN API
//

EXTERN void ShockLoadingInit(); 
EXTERN void ShockLoadingTerm(); 

// Nasty little hack we have to expose, to prevent recursive level trans
// in multiplayer:
EXTERN BOOL gbShockTransporting;

EXTERN void SwitchToShockInitGame(BOOL push); 
typedef struct sLoopInstantiator sLoopInstantiator; 
EXTERN sLoopInstantiator* DescribeShockInitGameMode(void); 

#endif // __SHKLDING_H
