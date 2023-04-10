/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpdoll.h,v 1.4 2000/01/31 09:58:25 adurant Exp $
#pragma once

#ifndef __SHKPDOLL_H
#define __SHKPDOLL_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockPaperDollInit(int which);
EXTERN void ShockPaperDollTerm(void);
EXTERN void ShockPaperDollDraw(void);
EXTERN bool ShockPaperDollHandleMouse(Point pt);
EXTERN bool ShockPaperDollDoubleClick(Point pos);
EXTERN bool ShockPaperDollDragDrop(Point pos, BOOL start);
EXTERN void ShockPaperDollStateChange(int which);
EXTERN bool ShockPaperDollCheckTransp(Point pt);

#endif