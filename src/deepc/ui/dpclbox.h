/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCLBOX_H
#define __DPCLBOX_H

extern "C" 
{
#include <event.h>
}

EXTERN void DPCLetterboxDraw(unsigned long inDeltaTicks);
EXTERN void DPCLetterboxInit(int which);
EXTERN void DPCLetterboxTerm(void);

EXTERN void DPCLetterboxSet(BOOL camera);
#endif  // __DPCLBOX_H