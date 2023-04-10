/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCAMMO_H
#define __DPCAMMO_H

extern "C" 
{
#include <event.h>
}

EXTERN void DPCAmmoDraw(unsigned long inDeltaTicks);
EXTERN void DPCAmmoInit(int which);
EXTERN void DPCAmmoTerm(void);
EXTERN bool DPCAmmoHandleMouse(Point pt);
EXTERN bool DPCAmmoCheckTransp(Point pt);
EXTERN void DPCAmmoRefreshButtons(void);
EXTERN void DPCAmmoStateChange(int which);

EXTERN void DPCAmmoReload(void);
EXTERN void DPCAmmoChangeTypes(void);
EXTERN void DPCAmmoPsiLevel(int button);

#endif  // __DPCAMMO_H