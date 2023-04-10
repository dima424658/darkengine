/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once  
#ifndef __DPCSAVUI_H
#define __DRKSAVUI_H

////////////////////////////////////////////////////////////
// DEEP COVER SAVE/LOAD UI
//

//
// Init/Term
//

EXTERN void DPCSaveUIInit(void);
EXTERN void DPCSaveUITerm(void); 

//
// Load panel
//
EXTERN void SwitchToDPCLoadGameMode(BOOL push); 
EXTERN void SwitchToDPCSaveGameMode(BOOL push); 


#endif // __DPCSAVUI_H
