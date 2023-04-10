/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once  
#ifndef __DPCMENU_H
#define __DPCMENU_H

////////////////////////////////////////////////////////////
// Deep Cover Metagame Menus
//

EXTERN void DPCMenusInit(void); 
EXTERN void DPCMenusTerm(void); 

//
// Deep Cover main menu 
//

EXTERN const struct sLoopInstantiator* DescribeDPCMainMenuMode(void); 
EXTERN void SwitchToDPCMainMenuMode(BOOL push); 

//
// Sim menu 
//

EXTERN const struct sLoopInstantiator* DescribeDPCSimMenuMode(void); 
EXTERN void SwitchToDPCSimMenuMode(BOOL push); 


#endif // __DPCMENU_H

