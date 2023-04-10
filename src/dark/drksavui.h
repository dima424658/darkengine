/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksavui.h,v 1.2 1998/10/23 16:48:12 DARRENL Exp $
#pragma once  
#ifndef __DRKSAVUI_H
#define __DRKSAVUI_H

////////////////////////////////////////////////////////////
// DARK SAVE/LOAD UI
//

//
// Init/Term
//

EXTERN void DarkSaveInitUI(void);
EXTERN void DarkSaveTermUI(void); 

//
// Load panel
//
EXTERN void SwitchToLoadGameMode(BOOL push); 
EXTERN void SwitchToSaveGameMode(BOOL push); 
EXTERN void SwitchToLoadBndMode(BOOL push); 
EXTERN void SwitchToSaveBndMode(BOOL push); 


#endif // __DRKSAVUI_H
