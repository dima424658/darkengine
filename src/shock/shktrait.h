/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktrait.h,v 1.3 2000/01/31 09:59:26 adurant Exp $
#pragma once

#ifndef __SHKTRAIT_H
#define __SHKTRAIT_H

#include <rect.h>

extern "C"
{
#include <event.h>
}

typedef enum eTrait;

EXTERN void ShockTraitInit(int which);
EXTERN void ShockTraitTerm(void);
EXTERN void ShockTraitDraw(void);
EXTERN bool ShockTraitHandleMouse(Point pt);
EXTERN void ShockTraitStateChange(int which);
EXTERN bool ShockTraitCheckTransp(Point pt);
EXTERN void ShockTraitDrawIcon(eTrait which, int dx, int dy);


#endif