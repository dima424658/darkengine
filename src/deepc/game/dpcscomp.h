/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCSCOMP_H
#define __DPCSCOMP_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCSecCompInit(int which);
EXTERN void DPCSecCompTerm(void);
EXTERN void DPCSecCompDraw(unsigned long inDeltaTicks);
EXTERN void DPCSecCompStateChange(int which);

#endif  // __DPCSCOMP_H