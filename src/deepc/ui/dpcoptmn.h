/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCOPTMN_H
#define __DPCOPTMN_H

//////////////////////////////////////////////////////////////
// OPTIONS PANEL MODE FOR DEEP COVER
//

EXTERN void SwitchToDPCOptionsMode(BOOL push); 
EXTERN const struct sLoopInstantiator* DescribeDPCOptionsMode(void); 


EXTERN void DPCOptionsMenuInit();
EXTERN void DPCOptionsMenuTerm();


#endif // __DPCOPTMN_H
