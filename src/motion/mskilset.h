/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mskilset.h,v 1.14 2000/01/29 13:22:20 adurant Exp $
#pragma once

#ifndef __MSKILSET_H
#define __MSKILSET_H

#ifdef __cplusplus ///////////////

#include <mctype.h>
#include <mctype_.h>
#include <motschem.h>
#include <tagdbin.h>
#include <motrtype.h>


class cMSkillSet
{
public:
   cMSkillSet() { m_ActorType=-1; }
   virtual ~cMSkillSet() {}

   void SetActorType(int type);

   // these are tags to be applied to every motion request
   void SetPersistentTags(const cTagSet *pTags);

   cMotionPlan *BuildPlanFromParams(const sMcMoveParams *pParams, \
      const sMcMotorState *pMotorState, const sMcMoveState *pMoveState, \
      IMotor *pMotor, cMotionCoordinator *pCoord);

   void Pose(const cTagSet *pTags, float frac, IMotor *pMotor);

private:
   int                  m_ActorType;
   cTagSet              m_PersistTags;
};

#endif // cplusplus

#endif
