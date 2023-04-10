/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmain.h,v 1.2 2000/01/31 09:57:53 adurant Exp $
#pragma once

#ifndef __SHKMAIN_H
#define __SHKMAIN_H

//
// Shock "Master Control" Loopmode.  
//
// Currently just  starts the intro cutscene and switches to main
// menu, but may do more in the future.  
//

EXTERN void ShockMasterModeInit(); 
EXTERN void ShockMasterModeTerm(); 


EXTERN struct sLoopInstantiator* DescribeShockMasterMode(void); 

enum eShockMasterModeState
{
   kMasterModeAppStart, 
   kMasterModeNewGame, 
}; 

// Set which of the above states the master mode will be in
EXTERN void ShockSetMasterMode(int state); 

#endif // __SHKMAIN_H

