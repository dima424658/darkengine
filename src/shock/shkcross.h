/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcross.h,v 1.2 2000/01/31 09:55:24 adurant Exp $
#pragma once

#ifndef __SHKCROSS_H
#define __SHKCROSS_H

#include <rect.h>

EXTERN void ShockCrosshairDraw(void);
EXTERN void ShockCrosshairInit(int which);
EXTERN void ShockCrosshairTerm(void);
EXTERN bool ShockCrosshairCheckTransp(Point pt);

#endif