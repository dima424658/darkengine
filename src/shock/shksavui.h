/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shksavui.h,v 1.2 1999/06/08 16:09:36 mahk Exp $
#pragma once  
#ifndef __DRKSAVUI_H
#define __DRKSAVUI_H

////////////////////////////////////////////////////////////
// DARK SAVE/LOAD UI
//

//
// Init/Term
//

EXTERN void ShockSaveUIInit(void);
EXTERN void ShockSaveUITerm(void); 

//
// Load panel
//
EXTERN void SwitchToShockLoadGameMode(BOOL push); 
EXTERN void SwitchToShockSaveGameMode(BOOL push); 


#endif // __DRKSAVUI_H
