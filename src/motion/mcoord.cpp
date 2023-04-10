/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mcoord.cpp,v 1.24 1999/03/02 20:18:20 mwhite Exp $
////
// XXX need to deal reasonably with maneuvers getting aborted and still 
// getting accurate end move state (can't just have delete call notify about
// completion because in setmotor and setcurrent want to delete both current
// and next).  Should velocity be stored with motor after all?
//
// XXX should figure out whether to build skill set at motor setting or
// coordinator creation or somewhere completely different.  currently set
// when motor is set.

#include <mcoord.h>
#include <mmanuver.h>
#include <mskilset.h>
#include <cfgdbg.h>
#include <config.h>
#include <tagfile.h>
#include <motmngr.h>

// must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//
//              GLOBAL FUNCTIONS
//
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////


IMotionCoordinator *MotSysCreateMotionCoordinator()
{
   return new cMotionCoordinator;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//
//              MOTION COORDINATOR CONSTRUCTOR/DESTRUCTOR
//
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

cMotionCoordinator::cMotionCoordinator() :
m_pMotor(NULL),
m_pCurrentManeuver(NULL),
m_pNextManeuver(NULL)
{
   ClearInterMnvrState();

   m_MotorStateChangeCallback=NULL;
   m_MotorStateChangeCallbackContext=NULL;
   m_ManeuverCompletionCallback=NULL;
   m_ManeuverCompletionCallbackContext=NULL;
   m_InMotorFrame=FALSE;
   m_ManeuverCompleted=FALSE;
   m_CanInterrupt=FALSE; 

   memset(&m_NextMoveState,0,sizeof(m_NextMoveState));
}

cMotionCoordinator::~cMotionCoordinator()
{
   if(m_pCurrentManeuver)
   {
      if(m_ManeuverCompletionCallback)
      {
         (*m_ManeuverCompletionCallback)(m_ManeuverCompletionCallbackContext,m_pCurrentManeuver,NULL);
      }
      delete m_pCurrentManeuver;
      m_pCurrentManeuver=NULL;
   }
   if(m_pNextManeuver)
   {
      delete m_pNextManeuver;
      m_pNextManeuver=NULL;
   }
   if(m_pMotor)
   {
      m_pMotor->SetMotorResolver(NULL);
   }
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//
//              MOTION COORDINATOR INTERFACE FUNCTIONS
//
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

// detatch from old motor
void cMotionCoordinator::EndMotor(IMotor *pMotor)
{
   if(m_pMotor)
   {
      m_pMotor->SetMotorResolver(NULL);
      m_pMotor=NULL;
   }
}

void cMotionCoordinator::StartMotor(IMotor *pMotor)
{
   m_pMotor=pMotor;

   ClearInterMnvrState();
   if(pMotor)
   {
      pMotor->SetMotorResolver((IMotorResolver *)this);
      m_SkillSet.SetActorType(pMotor->GetActorType());

      pMotor->GetTransform(&m_NextMoveState.position,&m_NextMoveState.facing);
      m_NextMoveState.speed=0;
      m_NextMoveState.direction=m_NextMoveState.facing;
   }

   // get rid of existing maneuvers, since they have wrong pMotor
   if(m_pCurrentManeuver)
   {
      m_pCurrentManeuver->SetMotor(NULL);
      if(m_ManeuverCompletionCallback)
      {
         (*m_ManeuverCompletionCallback)(m_ManeuverCompletionCallbackContext,m_pCurrentManeuver,NULL);
      }
      delete m_pCurrentManeuver;
      m_pCurrentManeuver=NULL;
   }
   if(m_pNextManeuver)
   {
      m_pNextManeuver->SetMotor(NULL);
      delete m_pNextManeuver;
      m_pNextManeuver=NULL;
   }
}

void cMotionCoordinator::SetPersistentTags(const cTagSet *pTags)
{
   if(!pTags)
   {
      m_SkillSet.SetPersistentTags(NULL);
      return;
   }
   m_SkillSet.SetPersistentTags(pTags);
}

IMotionPlan *cMotionCoordinator::BuildPlan(const sMcMoveState *pStartState, const sMcMoveParams *pParams)
{
   Assert_(m_pMotor);

#ifndef SHIP
   if(config_is_defined("MCoordTrace"))
   {
      mprintf("%d: ",m_pMotor->GetObjID()); // rest of spew done by mskilset
   }
#endif

   return m_SkillSet.BuildPlanFromParams(pParams,&m_MotorState,pStartState,m_pMotor,this);
}

void cMotionCoordinator::Pose(const cTagSet *pTags, float frac)
{
   m_SkillSet.Pose(pTags,frac,m_pMotor);
}

// 
eMCoordStatus cMotionCoordinator::GetStatus() const
{
   eMCoordStatus status;

   if(!m_pCurrentManeuver)
      status=kMCoord_Idle;
   else if(m_CanInterrupt)
      status=kMCoord_ActiveInterrupt;
   else
      status=kMCoord_ActiveBusy;
#ifndef SHIP
   if(status!=kMCoord_ActiveBusy && m_pMotor)
   {
      ConfigSpew("MnvrTrace",("%d Telling Client that Coord is interruptable\n",m_pMotor->GetObjID()));
   }
#endif
   return status;
}

void cMotionCoordinator::NotifyAboutManeuverCompletion(IManeuver *pMnvr, sMcMoveState& endState)
{
   AssertMsg(pMnvr==m_pCurrentManeuver,"weird coordinator maneuver");

   m_CanInterrupt=FALSE; // reset, since this is what old maneuver told us
   if(InMotorFrame())
   {
      m_ManeuverCompleted=TRUE;
      m_NextMoveState=endState;
   } else
   {
      StartNextManeuver();
   }
}

void cMotionCoordinator::StartNextManeuver()
{
   if(m_ManeuverCompletionCallback)
   {
      (*m_ManeuverCompletionCallback)(m_ManeuverCompletionCallbackContext,m_pCurrentManeuver,m_pNextManeuver);
   }

   delete m_pCurrentManeuver;

   if(m_pNextManeuver)
   {
      m_pCurrentManeuver=m_pNextManeuver;
      m_pNextManeuver=NULL;

// XXX need to figure out how this should work for real
//      m_pCurrentManeuver->GetExpectedEndMoveState(m_NextMoveState);
      m_pCurrentManeuver->Execute();
   } else
   {
      m_pCurrentManeuver=NULL;
   }
}

// Get rid of stored up maneuvers, since they were built under false pretenses
void cMotionCoordinator::NotifyAboutMotorStateChange()
{
   // this interrupts saveload.  hmm.  Not really important for already-executing
   // maneuver.  Just get rid of next one.  KJ 10/98
#if 0
   if(m_pCurrentManeuver)
   {
      if(m_ManeuverCompletionCallback)
      {
         (*m_ManeuverCompletionCallback)(m_ManeuverCompletionCallbackContext,m_pCurrentManeuver,NULL);
      }
      delete m_pCurrentManeuver;
      m_pCurrentManeuver=NULL;
   }
#endif
   if(m_pNextManeuver)
   {
      delete m_pNextManeuver;
      m_pNextManeuver=NULL;
   }
   // notify client, if callback was provided
   if(m_MotorStateChangeCallback)
      m_MotorStateChangeCallback(m_MotorStateChangeCallbackContext);
}

void cMotionCoordinator::NotifyAboutMotorDestruction()
{
   // don't want to end current motor, since invalid.  just
   // start the NULL one directly instead of calling SetMotor(NULL)
   StartMotor(NULL);
}

// Used to set the maneuver to be executed when the currently executing one is
// completed.  This is necessary because maneuver completion may occur between
// client NeedManeuver calls.  If no next maneuver is set, the coordinator will
// do nothing once te current one is completed until a maneuver is set.
//
void cMotionCoordinator::SetNextManeuver(IManeuver *pMnvr)
{
   if(!m_pCurrentManeuver)
   {
      m_pCurrentManeuver=pMnvr;
      if(!pMnvr)
         return;
//      m_pCurrentManeuver->GetExpectedEndMoveState(m_NextMoveState);
      AssertMsg(m_pMotor,"Cannot do maneuver without a motor");
      ConfigSpew("MnvrTrace",("Starting Maneuver\n"));
      m_pCurrentManeuver->Execute();
   } else
   {
      if(m_pNextManeuver)
         delete m_pNextManeuver;
      m_pNextManeuver=pMnvr;
   }
}

// Used when the client wants to interrupt the currently executing maneuver with
// a new one, instead of making the new maneuver get executed when the current
// one is done.   
//
void cMotionCoordinator::SetCurrentManeuver(IManeuver *pMnvr)
{
   if(m_pCurrentManeuver)
   {
      // tell current maneuver to finish executing, so it can do whatever
      // cleanup it needs to, and can store off state for next maneuver.
      // @NOTE: many maneuvers to motion stopping, defocusing etc in
      // destructor, not on finish, though Finish really is the right place
      // for them.

      m_pCurrentManeuver->Finish();

      if(m_ManeuverCompletionCallback)
      {
         (*m_ManeuverCompletionCallback)(m_ManeuverCompletionCallbackContext,m_pCurrentManeuver,pMnvr);
      }
      delete m_pCurrentManeuver;
   }

   m_pCurrentManeuver=pMnvr;

   if(m_pNextManeuver)
   {
      delete m_pNextManeuver;
      m_pNextManeuver = NULL;
   }
//   m_pCurrentManeuver->GetExpectedEndMoveState(m_NextMoveState);
   if(m_pCurrentManeuver)
   {
      AssertMsg(m_pMotor,"Cannot do maneuver without a motor");
      ConfigSpew("MnvrTrace",("Starting Maneuver\n"));
      m_pCurrentManeuver->Execute();
   }
}

// Used to get expected client state at the end of the currently executing 
// maneuver.  This state should be used for new plan building.
//
const sMcMoveState *cMotionCoordinator::GetNextEndState() const
{
   return &m_NextMoveState;
}

// NOTE: if controller ids change, some person should invalidate save games
void cMotionCoordinator::Load(ITagFile *pTagFile)
{
   // should only be called on pristine motion coordinator
   AssertMsg(!m_pCurrentManeuver&&!m_pNextManeuver,"loading game over busy motion coordinator!");

   // read inter-maneuver state
   ITagFile_Move(pTagFile,(char *)&m_MotorState,sizeof(m_MotorState));
   // @TODO: swizzle schema id

   // read controller id for currently executing maneuver.
   // if -1, then not doing maneuver currently
   int cid;
   ITagFile_Move(pTagFile,(char *)&cid,sizeof(cid));
   if(cid>=0)
   {
      IManeuver *pCurMan=g_ManeuverFactoryList[cid]->LoadManeuver(m_pMotor,this,pTagFile);
      m_pCurrentManeuver=pCurMan;
   }
   // @NOTE: next maneuver info not saved.  currently never used KJ 10/98
}

// @NOTE: this doesn't save nextmaneuver info.  Could add if we need it. KJ 
void cMotionCoordinator::Save(ITagFile *pTagFile)
{
   // write out inter-maneuver state
   ITagFile_Move(pTagFile,(char *)&m_MotorState,sizeof(m_MotorState));

   // write out controllers for maneuvers
   int cid=-1;
   if(m_pCurrentManeuver)
   {
      cid=m_pCurrentManeuver->GetAppData();      
      ITagFile_Move(pTagFile,(char *)&cid,sizeof(cid));
      m_pCurrentManeuver->Save(pTagFile);
   } else
   {
      ITagFile_Move(pTagFile,(char *)&cid,sizeof(cid));
   }
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//
//              MOTOR RESOLVER FUNCTIONS
//
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

void cMotionCoordinator::CalcEnvironmentEffect()
{
}

void cMotionCoordinator::CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel)
{
   if(m_pCurrentManeuver)
   {
      m_pCurrentManeuver->CalcCollisionResponse(pForces,nForces,pCurrentVel,pNewVel);
   } else
   {
      AssertMsg(FALSE,"Collision but no maneuver!");
      *pNewVel=*pCurrentVel;
   }
}

void cMotionCoordinator::NotifyAboutBeingStuck()
{
   AssertMsg(m_pCurrentManeuver,"no maneuver but got stuck!");

   if(m_pCurrentManeuver)
      m_pCurrentManeuver->NotifyAboutBeingStuck();
}

void cMotionCoordinator::NotifyAboutMotionAbortRequest()
{
   if(m_pCurrentManeuver)
      m_pCurrentManeuver->NotifyAboutMotionAbortRequest();
   else
      Warning(("MCoord: no current maneuver but told to abort!\n"));
}

void cMotionCoordinator::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   AssertMsg(m_pCurrentManeuver,"no maneuver but motion was playing!");

   if(m_pCurrentManeuver)
   {
      m_pCurrentManeuver->NotifyAboutMotionEnd(motionNum, frame, flags);
   }
}

//
// Motion status notification functions
//
void cMotionCoordinator::NotifyAboutFrameUpdateBegin()
{
   m_CanInterrupt=FALSE; // reset, since is instant

   m_InMotorFrame=TRUE;
}

void cMotionCoordinator::NotifyAboutFrameUpdateEnd()
{
   m_InMotorFrame=FALSE;
   if(m_ManeuverCompleted)
   {
      StartNextManeuver();
      m_ManeuverCompleted=FALSE;
   }
}

void cMotionCoordinator::NotifyAboutFrameFlags(const int flags)
{
   if(m_pCurrentManeuver)
      m_pCurrentManeuver->NotifyAboutFrameFlags(flags);
}

// this only lasts for one frame, so "instantaneously interruptable"
void cMotionCoordinator::NotifyThatCanInterrupt()
{
   ConfigSpew("MnvrTrace",("%d Coord is interruptable\n",m_pMotor->GetObjID()));
   m_CanInterrupt=TRUE;
}
