/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkalarm.h,v 1.4 2000/01/29 13:40:24 adurant Exp $
#pragma once

#ifndef __SHKALARM_H
#define __SHKALARM_H

extern "C" 
{
#include <event.h>
}

EXTERN void ShockAlarmDraw(void);
EXTERN void ShockAlarmInit(int which);
EXTERN void ShockAlarmTerm(void);
EXTERN void ShockHackIconDraw(void);
EXTERN void ShockHackIconInit(int which);
EXTERN void ShockHackIconTerm(void);

EXTERN void ShockAlarmAdd(int time);
EXTERN void ShockAlarmRemove(void);
EXTERN void ShockAlarmDisableAll(void);
#endif