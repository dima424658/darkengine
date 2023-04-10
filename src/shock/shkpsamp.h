/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsamp.h,v 1.3 2000/01/31 09:58:44 adurant Exp $
#pragma once

#ifndef __SHKPSAMP_H
#define __SHKPSAMP_H

extern "C"
{
#include <event.h>
}

EXTERN void ShockPsiAmpInit(int which);
EXTERN void ShockPsiAmpTerm(void);
EXTERN void ShockPsiAmpDraw(void);
EXTERN bool ShockPsiAmpHandleMouse(Point pt);
EXTERN void ShockPsiAmpStateChange(int which);
EXTERN bool ShockPsiAmpCheckTransp(Point pt);

#endif