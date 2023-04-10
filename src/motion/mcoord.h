/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mcoord.h,v 1.18 2000/01/31 09:50:09 adurant Exp $
#pragma once

#ifndef __MCOORD_H
#define __MCOORD_H

#include <mclntapi.h>
#include <mmresapi.h>
#include <motorapi.h>
#include <mskilset.h>
#include <mctype_.h>


// @TODO: rename this so "inter-maneuver state"-ish kind of thing.
// This should only be used as place for maneuvers to set things that
// next maneuver may want to know about.  Nobody else should set anything
// here.
struct sMcMotorState
{
   int   lastSchemaID;
   int   lastControllerID;
   ulong flags;
   float lastMotPercentCompleted;
};

class cMotionCoordinator: public IMotionCoordinator, public IMotorResolver
{
public:
   cMotionCoordinator();
   virtual ~cMotionCoordinator();

////////
// Motion Coordinator Functions
////////
   virtual void Load(ITagFile *pTagFile);
   virtual void Save(ITagFile *pTagFile);

   // these are tags to be applied to every motion and pose request
   virtual void SetPersistentTags(const cTagSet *pTags);

   virtual void SetMotor(IMotor *pMotor) { if(m_pMotor==pMotor) return; EndMotor(m_pMotor); StartMotor(pMotor); }
      
   virtual IMotionPlan *BuildPlan(const sMcMoveState *pStartState, const sMcMoveParams *pParams);


// this happens instantaneously, without blending or interrupting any
// currently executing maneuvers.  Coordinator chooses a motion
// (always that same one) that matches the tags, and poses the motor
// at the given fraction of the way through the motion.
   virtual void Pose(const cTagSet *pTags, float frac);

// 
   virtual eMCoordStatus GetStatus() const;

// Called by maneuver when it is completed
   virtual void NotifyAboutManeuverCompletion(IManeuver *pMnvr, sMcMoveState &endState);

// Called by outside systems when they move the creature/object without going through
// motion coordinator.
   virtual void NotifyAboutMotorStateChange();

// Called by outside systems when they delete the creature/motor without going through
// motion coordinator.
   virtual void NotifyAboutMotorDestruction();

// Used to set the maneuver to be executed when the currently executing one is
// completed.  This is necessary because maneuver completion may occur between
// client GetStatus calls.  If no next maneuver is set, the coordinator will
// do nothing once te current one is completed until a maneuver is set.
//
   virtual void SetNextManeuver(IManeuver *pMnvr);

// Used when the client wants to interrupt the currently executing maneuver with
// a new one, instead of making the new maneuver get executed when the current
// one is done.   
//
   virtual void SetCurrentManeuver(IManeuver *pMnvr);

// Used to get expected client state at the end of the currently executing 
// maneuver.  This state should be used for new plan building.
//
   virtual const sMcMoveState *GetNextEndState() const;

// Set callback for coordinator to use when motor it is controlling gets
// moved around by other systems (editor, scripts etc).
   virtual void SetMotorStateChangeCallback(fMcMotorStateChangeCallback callback, void *context)
      { m_MotorStateChangeCallback=callback; m_MotorStateChangeCallbackContext=context; }

// Set callback for coordinator to use when it completes a maneuver
   virtual void SetManeuverCompletionCallback(fMcManeuverCompletionCallback callback, void *context)
      { m_ManeuverCompletionCallback=callback; m_ManeuverCompletionCallbackContext=context; }


////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect();
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel);
   virtual void NotifyAboutBeingStuck();
   virtual void NotifyAboutMotionAbortRequest();

//
// Motion status notification functions
//
   virtual void NotifyAboutFrameUpdateBegin();
   virtual void NotifyAboutFrameUpdateEnd();
   virtual void NotifyAboutFrameFlags(const int);
   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags);
   // this only lasts for one frame
   virtual void NotifyThatCanInterrupt();

////////
// Maneuver-called functions
////////
   // @TODO: come up with better name for this
   // set state by getting ptr and futzing with it
   virtual sMcMotorState *GetInterMnvrState() { return &m_MotorState; }
   virtual void ClearInterMnvrState()
      { m_MotorState.lastSchemaID=-1;
        m_MotorState.lastControllerID=-1; 
        m_MotorState.flags=NULL;
        m_MotorState.lastMotPercentCompleted=0;
      }

private:
   void EndMotor(IMotor *pMotor);
   void StartMotor(IMotor *pMotor);
   BOOL InMotorFrame() { return m_InMotorFrame; }
   void StartNextManeuver();

   IMotor *m_pMotor;
   cMSkillSet m_SkillSet; 
   IManeuver *m_pCurrentManeuver;
   IManeuver *m_pNextManeuver;
   sMcMotorState m_MotorState;
   sMcMoveState m_NextMoveState;
   fMcMotorStateChangeCallback m_MotorStateChangeCallback;
   void *m_MotorStateChangeCallbackContext;
   fMcManeuverCompletionCallback m_ManeuverCompletionCallback;
   void *m_ManeuverCompletionCallbackContext;
   BOOL m_InMotorFrame;
   BOOL m_ManeuverCompleted;
   BOOL m_CanInterrupt;
// need to add state stuff for maneuvers
};

#endif
