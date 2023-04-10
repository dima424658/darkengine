/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shksecur.h,v 1.2 2000/01/31 09:59:11 adurant Exp $
#pragma once

#ifndef __SHKSECUR_H
#define __SHKSECUR_H

extern "C"
{
#include <event.h>
}

EXTERN void ShockSecurityInit(int which);
EXTERN void ShockSecurityTerm(void);
EXTERN void ShockSecurityDraw(void);
EXTERN bool ShockSecurityHandleMouse(Point pt);
EXTERN void ShockSecurityStateChange(int which);
EXTERN bool ShockSecurityCheckTransp(Point pos);

#endif