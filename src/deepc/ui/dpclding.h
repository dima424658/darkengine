/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCLDING_H
#define __DPCLDING_H

////////////////////////////////////////////////////////////
// DEEP COVER LOADING SCREEN API
//

EXTERN void DPCLoadingInit(); 
EXTERN void DPCLoadingTerm(); 

// Nasty little hack we have to expose, to prevent recursive level trans
// in multiplayer:
EXTERN BOOL gbDPCTransporting;

EXTERN void SwitchToDPCInitGame(BOOL push); 
typedef struct sLoopInstantiator sLoopInstantiator; 
EXTERN sLoopInstantiator* DescribeDPCInitGameMode(void); 
EXTERN BOOL DPCIsToGameModeGUID(REFGUID rguid);
#endif // __DPCLDING_H
