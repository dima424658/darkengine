/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCHAZRD_H
#define __DPCHAZRD_H

extern "C" 
{
#include <event.h>
}

EXTERN void DPCRadDraw(unsigned long inDeltaTicks);
EXTERN void DPCRadInit(int which);
EXTERN void DPCRadTerm(void);
#endif  // !__DPCHAZRD_H