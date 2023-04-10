/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCMAIN_H
#define __DPCMAIN_H

//
// Deep Cover "Master Control" Loopmode.  
//
// Currently just  starts the intro cutscene and switches to main
// menu, but may do more in the future.  
//

EXTERN void DPCMasterModeInit(); 
EXTERN void DPCMasterModeTerm(); 
EXTERN void UnwindToMissionLoop(); 


EXTERN struct sLoopInstantiator* DescribeDPCMasterMode(void); 

enum eDPCMasterModeState
{
   kMasterModeAppStart, 
   kMasterModeNewGame, 
}; 

// Set which of the above states the master mode will be in
EXTERN void DPCSetMasterMode(int state); 

#endif // __DPCMAIN_H

