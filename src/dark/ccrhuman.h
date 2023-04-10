/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/ccrhuman.h,v 1.2 2000/01/31 09:39:29 adurant Exp $
#pragma once

#ifndef __CCRHUMAN_H
#define __CCRHUMAN_H

// Joints
#define LTOE      0
#define RTOE      1
#define LANKLE    2
#define RANKLE    3
#define LKNEE     4
#define RKNEE     5
#define LHIP      6
#define RHIP      7
#define BUTT      8
#define NECK      9
#define LSHLDR   10
#define RSHLDR   11
#define LELBOW   12
#define RELBOW   13
#define LWRIST   14
#define RWRIST   15
#define LFINGER  16
#define RFINGER  17
#define ABDOMEN  18
#define HEAD     19
#define LSHLDRIN 20
#define RSHLDRIN 21
#define LWEAP    22
#define RWEAP    23

#ifdef __cplusplus

#include <creature.h>

EXTERN cCreatureFactory *g_pHumanoidCreatureFactory;

class cHumanoidCreature : public cCreature
{
public:
   cHumanoidCreature(int type,ObjID objID) :
      cCreature(type,objID) {}

   cHumanoidCreature(int type,ObjID objID, fCreatureReadWrite func, int version) : 
      cCreature(type,objID,func,version) {}
   virtual ~cHumanoidCreature() {}

   virtual int GetFocusJoint() { return NECK; }
   virtual void GetFocusRotOffset(quat *pDest, const quat *pBase, const quat *rots);
};

inline void cHumanoidCreature::GetFocusRotOffset(quat *pDest, const quat *pBase, const quat *rots)
{
   quat q;

   quat_mul(&q,(quat *)&rots[BUTT],(quat *)pBase);
   quat_mul(pDest,(quat *)&rots[ABDOMEN],(quat *)&q);
}

#endif // cplusplus


#endif // ccrhuman
