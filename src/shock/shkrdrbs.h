/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrdrbs.h,v 1.4 2000/01/31 09:58:57 adurant Exp $
#pragma once

#ifndef __SHKRDRBS_H
#define __SHKRDRBS_H

#include <rect.h>
#include <simtime.h>

enum eRadarStatus {kRadarOff, kRadarOn,};
enum eTargetType { kTargetNone, kTargetEnemy, kTargetPowerup, };

#define kMaxRadarTargets   64

class cRadar
{
public:
   cRadar(void);

   void On(void);
   void Off(void);
   void Scan(void);
   void Status(int *pTargetNum, Point **ppTargetPts, int **ppTargetData);

private:
   eRadarStatus m_status;
   int m_numTargets; 
   mxs_vector m_targetVecs[kMaxRadarTargets]; // raw unrotated target data
   Point m_targetPts[kMaxRadarTargets];       // player relative target points
   tSimTime m_lastScanTime;
   int m_targetData[kMaxRadarTargets];
};

extern cRadar g_radar;


#endif