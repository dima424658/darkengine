/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhud.h,v 1.3 2000/01/31 09:56:41 adurant Exp $
#pragma once

#ifndef __SHKHUD_H
#define __SHKHUD_H

#include <objtype.h>

EXTERN void ShockHUDDraw(void);
EXTERN void ShockHUDInit(int which);
EXTERN void ShockHUDTerm(void);
EXTERN void ShockHUDDrawRect(Rect r, ObjID o);

EXTERN void ShockHUDClear();

#endif