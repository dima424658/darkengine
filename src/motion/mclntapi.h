/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mclntapi.h,v 1.22 2000/01/29 13:22:12 adurant Exp $
#pragma once

// This file declares the interface to the motion system to be used by
// higher-level systems.  The basic idea is that a client uses the
// MotionCoordinator to build plans to accomplish goals, and then
// feeds maneuvers from that plan to the MotionCoordinator when the coordinator
// needs them.  The motion system is a passive server from the perspective of
// the client.  The client can look at the plan built by the MotionCoordinator
// to decide if it likes it, and make adjustments to the plan as necessary.
//
// The client of the motion system is the AI cerebellum (see aicbllm.cpp)

#ifndef __MCLNTAPI_H
#define __MCLNTAPI_H

#include <mctype.h>
#include <matrixs.h>
#include <motrtype.h>
#include <label.h>
#include <tagdbt.h>
#include <ctagset.h>
#include <objtype.h> // for focus obj
#include <tagfile.h>
#include <fix.h>

// Maneuvers are the atomic units of action.
//
class IManeuver
{
public:
//// destructor
   virtual ~IManeuver() {}

////////
// Motion System Client Functions
////////
   virtual IManeuver *GetPrevManeuver()=0;
   virtual IManeuver *GetNextManeuver()=0;
   virtual void GetExpectedEndMoveState(sMcMoveState& moveState)=0;
   virtual void SetAppData(ulong data)=0;
   virtual ulong GetAppData()=0;

////////
// Motion Coordinator Functions
////////
   virtual void Execute()=0;
   virtual void Finish()=0;
   virtual void SetMotor(IMotor *pMotor)=0;
   virtual void Save(ITagFile *pTagFile)=0;

////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect()=0;
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel)=0;
   virtual void NotifyAboutBeingStuck()=0;
   virtual void NotifyAboutMotionAbortRequest()=0;
   virtual void NotifyAboutFrameFlags(const int)=0;
   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)=0;
};

// A plan is composed of a sequence of maneuvers.
//
class IMotionPlan
{
public:
   virtual IManeuver *GetFirstManeuver()=0;
   virtual IManeuver *GetLastManeuver()=0;
   virtual IManeuver *PopFirstManeuver()=0;
};

// The is the motion system, as far as the client is concerned.
// There should be exactly one MotionCoordinator per entity to be controlled.
//

//
// Status of motion coordinator.  Returned by GetStatus
//
enum eMCoordStatus
{
   kMCoord_Idle,
   kMCoord_ActiveInterrupt,
   kMCoord_ActiveBusy,
   kMCoord_Error,
   kMCoord_IntMax=0xffffffff,
};

// XXX are two maneuvers - next and current - really necessary?

class IMotionCoordinator
{
public:
//// destructor
   virtual ~IMotionCoordinator() {}

   virtual void Load(ITagFile *pTagFile)=0;
   virtual void Save(ITagFile *pTagFile)=0;

   virtual void SetMotor(IMotor *pMotor)=0;

   // these are tags to be applied to every motion request
   virtual void SetPersistentTags(const cTagSet *pTags)=0;

   virtual IMotionPlan *BuildPlan(const sMcMoveState *pStartState, const sMcMoveParams *pParams)=0;

// this happens instantaneously, without blending or interrupting any
// currently executing maneuvers.  Coordinator chooses a motion
// (always that same one) that matches the tags, and poses the motor
// at the given fraction of the way through the motion.
   virtual void Pose(const cTagSet *pTags, float frac)=0;

//
   virtual eMCoordStatus GetStatus() const=0;

// Used to set the maneuver to be executed when the currently executing one is
// completed.  This is necessary because maneuver completion may occur between
// client GetStatus calls.  If no next maneuver is set, the coordinator will
// do nothing once te current one is completed until a maneuver is set.
//
   virtual void SetNextManeuver(IManeuver *pMnvr)=0;

// Used when the client wants to interrupt the currently executing maneuver with
// a new one, instead of making the new maneuver get executed when the current
// one is done.
//
   virtual void SetCurrentManeuver(IManeuver *pMnvr)=0;

// Used to get expected client state at the end of the currently executing
// maneuver.  This state should be used for new plan building.
//
   virtual const sMcMoveState *GetNextEndState() const=0;

// Set callback for coordinator to use when motor it is controlling gets
// moved around by other systems (editor, scripts etc).
   virtual void SetMotorStateChangeCallback(fMcMotorStateChangeCallback callback, void *context)=0;

// Set callback for coordinator to use when it completes a maneuver
   virtual void SetManeuverCompletionCallback(fMcManeuverCompletionCallback callback, void *context)=0;

   // this only lasts for one frame, so "instantaneously interruptable"
   virtual void NotifyThatCanInterrupt()=0;
};


struct sMcMoveState
{
   mxs_vector  position;
   mxs_real    speed;
   mxs_angvec  facing;
   mxs_angvec  direction;
};

#define kMotParmFlag_Direction 0x4
#define kMotParmFlag_ExactSpeed 0x10
#define kMotParmFlag_Facing 0x20
#define kMotParmFlag_Duration 0x40
#define kMotParmFlag_Position 0x80
#define kMotParmFlag_Name     0x100
#define kMotParmFlag_MotNum   0x400
#define kMotParmFlag_Distance 0x1000
#define kMotParmFlag_FocusObj 0x2000

struct sMcMoveParams
{
   sMcMoveParams()
    : mask(0)
   {
	  turnspeed = 0x3800;
   }

   cTagSet     tags;
   ulong       mask;    // says which remaining fields to use and which to ignore
// locomotion fields
   mxs_angvec  direction;
   mxs_real    distance; // used to modify when stride length when length>distance
   mxs_real    exactSpeed; // quantitative speed value
   mxs_angvec  facing;
   ulong       duration;
   mxs_vector  position;
// single action fields
   char *   name;
   int      motionNum;
   ObjID    focus;
   fix		   turnspeed; // The maximum angle speed to turn the body.
};

IMotionCoordinator *MotSysCreateMotionCoordinator();

#endif
