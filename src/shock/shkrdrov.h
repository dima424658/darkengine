/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrdrov.h,v 1.5 2000/01/31 09:58:58 adurant Exp $
#pragma once

#ifndef __SHKRDROV_H
#define __SHKRDROV_H

extern "C"
{
#include <event.h>
}

const int kRadarOverlayWidth = 128;
const int kRadarOverlayHeight = 128;
const int kRadarHalfWidth = (kRadarOverlayWidth-1)/2;
const int kRadarHalfHeight = (kRadarOverlayHeight-1)/2;

EXTERN void ShockRadarInit(int which);
EXTERN void ShockRadarTerm(void);
EXTERN void ShockRadarDraw(void);
EXTERN bool ShockRadarHandleMouse(Point pt);

#endif