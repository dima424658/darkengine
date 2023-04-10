/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktrpsi.h,v 1.2 2000/01/31 09:59:29 adurant Exp $
#pragma once

#ifndef __SHKTRPSI_H
#define __SHKTRPSI_H

#include <rect.h>

EXTERN void ShockBuyPsiInit(int which);
EXTERN void ShockBuyPsiTerm(void);
EXTERN void ShockBuyPsiDraw(void);
EXTERN bool ShockBuyPsiHandleMouse(Point pt);
EXTERN void ShockBuyPsiStateChange(int which);

#endif