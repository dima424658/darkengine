/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrplyr.cpp,v 1.13 1999/08/05 17:10:19 Justin Exp $

#include <config.h>
#include <mmanuver.h>
#include <mcoord.h>
#include <wrtype.h>
#include <portal.h>
#include <mvrutils.h>
#include <motdesc.h>
#include <mclntapi.h>
#include <plyrvmot.h>
#include <mschutil.h>
#include <mschbase.h>
#include <ghostmvr.h>

      // must be last header
#include <dbmem.h>

class cPlayerManeuverFactory: public cManeuverFactory
{
public:

   virtual cMotionPlan *CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      
};

class cPlayerManeuver: public cManeuver
{
public:
   cPlayerManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams& params);
   ~cPlayerManeuver();

////////
// Motion System Client Functions
////////
   virtual void GetExpectedEndMoveState(sMcMoveState& moveState);

////////
// Motion Coordinator Functions
////////
   virtual void Execute();
    
   virtual void Finish();

////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect() {}
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel) \
      { MvrCalcSlidingCollisionResponse(pForces,nForces,pCurrentVel,pNewVel); }
   virtual void NotifyAboutBeingStuck() { m_pMotor->StopMotion(m_MotionNum); }

   virtual void NotifyAboutFrameFlags(const int flags) { MvrProcessStandardFlags(m_pMotor, m_pCoord, flags); }

   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags);
   virtual void NotifyAboutMotionAbortRequest();

private:
   int m_MotionNum;
   int m_SynchMotion;
   mxs_vector m_PosOffset;
   mxs_angvec m_AngOffset;
   float m_Duration;
   float m_TimeScale;
};
                         
cPlayerManeuverFactory g_PlayerManeuverFactory;
EXTERN cManeuverFactory *g_pPlayerManeuverFactory=&g_PlayerManeuverFactory;


cMotionPlan *cPlayerManeuverFactory::CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cPlayerManeuver *pMnvr;

   pMnvr = new cPlayerManeuver(pMotor, pCoord, pSchema, params);

   AssertMsg(pMnvr,"could not alloc maneuver");

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// Single MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define MAX_INTERESTED_MOTIONS 10

cPlayerManeuver::cPlayerManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams& params)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   

   m_MotionNum=-1;
   if (IsRemoteGhost(pMotor->GetObjID()))
   {
      m_MotionNum = GetGhostMotionNumber();

#ifndef SHIP
      if (config_is_defined("ghost_motion_spew"))
      {
         mprintf("rPlyr: Remote ghost %d playing schema %d, motion %d\n", 
                 pMotor->GetObjID(), pSchema->GetSchemaID(), m_MotionNum);
      }
#endif
   }
   if (m_MotionNum==-1)  // not a remoteghost, or remghost didnt want to deal 
   {
      if(params.mask&kMotParmFlag_MotNum)
      {
         m_MotionNum=params.motionNum;
      } else
      {
         if(!pSchema->GetRandomMotionInRange(1,pSchema->NumMotions()-1,&m_MotionNum))
            m_MotionNum=-1;
      }

      if (IsLocalGhost(pMotor->GetObjID()))
      {
#ifndef SHIP
         if (config_is_defined("ghost_motion_spew"))
         {
            mprintf("rPlyr: Local ghost %d informing remote about schema %d, motion %d\n", 
                    pMotor->GetObjID(), pSchema->GetSchemaID(), m_MotionNum);
         }
#endif
         GhostSendMoCap(pMotor->GetObjID(),pSchema->GetSchemaID(),m_MotionNum,FALSE);
      }
   }


   if(!pSchema->GetDuration(&m_Duration))
      m_Duration=0;
   if(!pSchema->GetTimeWarp(&m_TimeScale))
      m_TimeScale=0;

   // get pos offset stuff from schema
   if(!MSchUGetPlayerPosOffset(pSchema,&m_PosOffset,&m_AngOffset))
   {
      Warning(("Could not get player pos offset for schema!\n"));
      mx_zero_vec(&m_PosOffset);
      m_AngOffset.tx=0;
      m_AngOffset.ty=0;
      m_AngOffset.tz=0;
   }

   // this should always be first motion in player arm motion schema
   if(!pSchema->GetMotion(0,&m_SynchMotion))
   {
      Warning(("could not get cam synch motion\n"));
      m_SynchMotion=-1;
   }
}

cPlayerManeuver::~cPlayerManeuver()
{
   // need to check if motor valid, since SetMotor(NULL) may have been
   // called.
   if(!m_pMotor)
      return;

   // Stop motion if one is playing

   if(m_MotionNum>=0)
      m_pMotor->StopMotion(m_MotionNum);
   m_pMotor->StopMotion(m_SynchMotion);
}

void cPlayerManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   AssertMsg(FALSE,"GetExpectedEndMoveState not implemented for player maneuver");
}

void cPlayerManeuver::Execute()
{
   int virtMotion=m_SynchMotion;

   if(!m_pMotor)
   {
      sMcMoveState state;
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
      return;
   }

   PlyrVMotSetOffsets(&m_PosOffset,&m_AngOffset);
   // XXX NOTE: fixed duration doesn't work with mo-caps
   if(m_MotionNum>=0)
   {
      mps_motion_param param;

      param.flags=NULL;

      if(m_TimeScale)
      {
         param.flags|=MP_DURATION_SCALE;
         param.duration_scalar=m_TimeScale;
      }
      m_pMotor->StartMotionWithParam(m_MotionNum,&param);
   } 
   if(m_Duration<=0)
   {
      m_pMotor->StartMotion(virtMotion);
   } else
   {
      mps_motion_param param;

      param.flags=MP_FIXED_DURATION;
      param.fixed_duration=m_Duration*1000; // uses millisec

      m_pMotor->StartMotionWithParam(virtMotion,&param);
   }
   // note if motion number not valid, this maneuver will not auto-terminate
}

void cPlayerManeuver::Finish()
{
   if(!m_pMotor)
   {
      sMcMoveState state;
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
      return;
   }

   if(m_MotionNum<0)
   {
      sMcMoveState state;
      int virtMotion=m_SynchMotion;

      m_pMotor->StopMotion(virtMotion);
      m_pCoord->NotifyAboutManeuverCompletion(this, state);
   }
   // otherwise will auto-terminate when motion ends anyway
}


// XXX need to rework this end state stuff
void cPlayerManeuver::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   sMcMoveState state;

   AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
//   m_pMotor->StopMotion(virtMotion); // XXX hopefully just stopping it on deletion will be enough
               // there's a weird glitching bug, where mp library things that
               // overlay hasn't ended, but base motion has.

   m_pCoord->NotifyAboutManeuverCompletion(this,state);
}

// XXX need to rework this end state stuff
void cPlayerManeuver::NotifyAboutMotionAbortRequest()
{
   // need to check if motor valid, since SetMotor(NULL) may have been
   // called.
   if(!m_pMotor)
      return;

   // Stop motion if one is playing
   if(m_MotionNum>=0)
      m_pMotor->StopMotion(m_MotionNum);
   m_pMotor->StopMotion(m_SynchMotion);

   sMcMoveState state;
   m_pCoord->NotifyAboutManeuverCompletion(this,state);
}
