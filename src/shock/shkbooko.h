/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkbooko.h,v 1.5 2000/01/29 13:40:31 adurant Exp $
#pragma once

#ifndef __SHKBOOKO_H
#define __SHKBOOKO_H

extern "C"
{
#include <event.h>
}

EXTERN void ShockBookInit(int which);
EXTERN void ShockBookTerm(void);
EXTERN void ShockBookDraw(void);
EXTERN bool ShockBookHandleMouse(Point pos);
EXTERN void ShockBookStateChange(int which);

EXTERN void ShockBookDisplay(int level, int which);
#endif