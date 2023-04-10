/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkoptmn.h,v 1.2 2000/01/31 09:58:15 adurant Exp $
#pragma once

#ifndef __SHKOPTMN_H
#define __SHKOPTMN_H

//////////////////////////////////////////////////////////////
// OPTIONS PANEL MODE FOR SHOCK 2
//

EXTERN void SwitchToShockOptionsMode(BOOL push); 
EXTERN const struct sLoopInstantiator* DescribeShockOptionsMode(void); 


EXTERN void ShockOptionsMenuInit();
EXTERN void ShockOptionsMenuTerm();


#endif // __SHKOPTMN_H
