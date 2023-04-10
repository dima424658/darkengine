/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCHRM_H
#define __DPCHRM_H

extern "C"
{
#include <event.h>
}

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // ! _OBJTYPE_H

EXTERN void DPCHRMInit(int which);
EXTERN void DPCHRMTerm(void);
EXTERN void DPCHRMDraw(unsigned long inDeltaTicks);
EXTERN bool DPCHRMHandleMouse(Point pt);
EXTERN void DPCHRMStateChange(int which);

EXTERN void DPCHRMDisplay(ObjID o, int mode, BOOL use_psi = FALSE, int bonus = 0);
EXTERN int  DPCFindTechType(ObjID obj);

#endif  // __DPCHRM_H