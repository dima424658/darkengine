/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdebrf.h,v 1.1 1999/03/31 14:05:50 XEMU Exp $
#pragma once  
#ifndef __SHKDEBRF_H
#define __SHKDEBRF_H

////////////////////////////////////////////////////////////
// DEBRIEF PANEL
//

EXTERN void ShockDebriefPanelInit(void); 
EXTERN void ShockDebriefPanelTerm(void); 

//
// Debrief panel
//

EXTERN const struct sLoopInstantiator* DescribeShockDebriefMode(void); 
EXTERN void SwitchToShockDebriefMode(BOOL push, int mission); 

#endif 

