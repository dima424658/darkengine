/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkradar.h,v 1.3 2000/01/31 09:58:56 adurant Exp $
#pragma once

#ifndef __SHKRADAR_H
#define __SHKRADAR_H

#include <rect.h>

void RadarOn(void);
void RadarOff(void);
void RadarScan(void);
void RadarStatus(int *pTargetsNum, Point **ppTargetPts, int **ppTargetData);

#endif