/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhrm.h,v 1.6 2000/01/31 09:56:40 adurant Exp $
#pragma once

#ifndef __SHKHRM_H
#define __SHKHRM_H

extern "C"
{
#include <event.h>
}

#include <objtype.h>

EXTERN void ShockHRMInit(int which);
EXTERN void ShockHRMTerm(void);
EXTERN void ShockHRMDraw(void);
EXTERN bool ShockHRMHandleMouse(Point pt);
EXTERN void ShockHRMStateChange(int which);

EXTERN void ShockHRMDisplay(ObjID o, int mode, BOOL use_psi = FALSE, int bonus = 0);
EXTERN int ShockFindTechType(ObjID obj);

#endif