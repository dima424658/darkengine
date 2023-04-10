/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mmresapi.h,v 1.5 2000/01/31 09:50:54 adurant Exp $
#pragma once

#ifndef __MMRESAPI_H
#define __MMRESAPI_H

#include <motrtype.h>
#include <matrixs.h>

typedef class IMotorResolver IMotorResolver;

class IMotorResolver
{
public:
//
// Physics resolving functions
//
   virtual void CalcEnvironmentEffect()=0;
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel)=0;
   virtual void NotifyAboutBeingStuck()=0;
   virtual void NotifyAboutMotionAbortRequest()=0;

//
// Motor status notification functions
//
   virtual void NotifyAboutMotorStateChange()=0;
   virtual void NotifyAboutMotorDestruction()=0;

//
// Motion status notification functions
//
   virtual void NotifyAboutFrameUpdateBegin()=0;
   virtual void NotifyAboutFrameUpdateEnd()=0;
   virtual void NotifyAboutFrameFlags(const int)=0;
   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)=0;
};

#endif
