/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhcomp.h,v 1.2 2000/01/31 09:56:37 adurant Exp $
#pragma once

#ifndef __SHKHCOMP_H
#define __SHKHCOMP_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockComputerInit(int which);
EXTERN void ShockComputerTerm(void);
EXTERN void ShockComputerDraw(void);
EXTERN void ShockComputerStateChange(int which);

#endif