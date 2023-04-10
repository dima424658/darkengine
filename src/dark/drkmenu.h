/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmenu.h,v 1.1 1998/09/22 14:19:46 mahk Exp $
#pragma once  
#ifndef __DRKMENU_H
#define __DRKMENU_H

////////////////////////////////////////////////////////////
// DARK METAGAME MENUS
//

EXTERN void DarkMenusInit(void); 
EXTERN void DarkMenusTerm(void); 

//
// Main menu 
//

EXTERN const struct sLoopInstantiator* DescribeMainMenuMode(void); 
EXTERN void SwitchToMainMenuMode(BOOL push); 

//
// Sim menu 
//

EXTERN const struct sLoopInstantiator* DescribeSimMenuMode(void); 
EXTERN void SwitchToSimMenuMode(BOOL push); 


#endif // __DRKMENU_H

