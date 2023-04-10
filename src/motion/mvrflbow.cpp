/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrflbow.cpp,v 1.8 1998/05/13 13:00:55 kate Exp $

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

// must be last header
#include <dbmem.h>

class cFlexBowManeuverFactory: public cManeuverFactory
{
public:

   virtual cMotionPlan *CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      
};

class cFlexBowManeuver: public cManeuver
{
public:
   cFlexBowManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams& params);
   ~cFlexBowManeuver();

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

   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags);

private:
   int m_MotionNum;
   int m_FlexBowMotion;
   mxs_vector m_PosOffset;
   mxs_angvec m_AngOffset;
   float m_Duration;
   float m_TimeScale;
   float m_FlexDuration;
   mxs_ang m_MaxFlex;
};
                         
cFlexBowManeuverFactory g_FlexBowManeuverFactory;
EXTERN cManeuverFactory *g_pFlexBowManeuverFactory=&g_FlexBowManeuverFactory;


cMotionPlan *cFlexBowManeuverFactory::CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cFlexBowManeuver *pMnvr;

   pMnvr = new cFlexBowManeuver(pMotor, pCoord, pSchema, params);

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

EXTERN float g_FlexDuration;

EXTERN quat g_FlexTopStartRot;
EXTERN quat g_FlexTopEndRot;
EXTERN quat g_FlexBotStartRot;
EXTERN quat g_FlexBotEndRot;

#define MAX_INTERESTED_MOTIONS 10

#define min(x,y) ((x)<(y)?(x):(y))

cFlexBowManeuver::cFlexBowManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams& params)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   

   if(params.mask&kMotParmFlag_MotNum)
   {
      m_MotionNum=params.motionNum;
   } else
   {
      if(!pSchema->GetRandomMotionInRange(1,pSchema->NumMotions()-1,&m_MotionNum))
         m_MotionNum=-1;
   }

   // get pos offset stuff from schema
   if(!MSchUGetPlayerPosOffset(pSchema,&m_PosOffset,&m_AngOffset))
   {
      Warning(("Could not get player pos offset for schema!\n"));
      mx_zero_vec(&m_PosOffset);
      m_AngOffset.tx=0;
      m_AngOffset.ty=0;
      m_AngOffset.tz=0;
   }

   if(!pSchema->GetDuration(&m_Duration))
      m_Duration=0;
   if(!pSchema->GetTimeWarp(&m_TimeScale))
      m_TimeScale=0;
   sMPlayerSkillData skillData;
   // get flex stuff from skill data
   if(!MSchUGetPlayerSkillData(pSchema,&skillData))
   {
      AssertMsg(FALSE,"No player skill data for schema");
   }
   m_FlexDuration=skillData.flexDuration;
   m_MaxFlex=degrees_to_fixang(skillData.flexDegree);

   // this should always be first motion in bow motion schema
   if(!pSchema->GetMotion(0,&m_FlexBowMotion))
   {
      Warning(("could not get flex bow motion\n"));
      m_FlexBowMotion=-1;
   }
}

cFlexBowManeuver::~cFlexBowManeuver()
{
   // need to check m_pMotor in case SetMotor(NULL) was called.
   if(!m_pMotor)
      return;

   // Stop motion if one is playing
   if(m_MotionNum>=0)
      m_pMotor->StopMotion(m_MotionNum);
   m_pMotor->StopMotion(m_FlexBowMotion);
}

void cFlexBowManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   AssertMsg(FALSE,"GetExpectedEndMoveState not implemented for player maneuver");
}

void cFlexBowManeuver::Execute()
{
   int virtMotion=m_FlexBowMotion;

   if(!m_pMotor)
   {
      sMcMoveState state;
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
      return;
   }

   PlyrVMotSetOffsets(&m_PosOffset,&m_AngOffset);
   // XXX NOTE: fixed duration doesn't work with mo-caps
   // note if motion number not valid, this maneuver will not auto-terminate
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
   
   // setup bow flexing
   mxs_angvec maxFlex={0,0,0};
   mxs_matrix mat;

   if(m_MaxFlex)
   {
      maxFlex.tx=m_MaxFlex;
      mx_ang2mat(&mat,&maxFlex);
      quat_from_matrix(&g_FlexTopEndRot,&mat);
      mx_transeq_mat(&mat); // invert
      quat_from_matrix(&g_FlexBotEndRot,&mat);
   } else
   {
      quat_identity(&g_FlexTopEndRot);
      quat_identity(&g_FlexBotEndRot);
   }

   quat_identity(&g_FlexTopStartRot);
   quat_identity(&g_FlexBotStartRot);
   g_FlexDuration=m_FlexDuration;
}

void cFlexBowManeuver::Finish()
{
   if(m_MotionNum<0)
   {
      sMcMoveState state;
      int virtMotion=m_FlexBowMotion;
   
      m_pMotor->StopMotion(virtMotion);
      m_pCoord->NotifyAboutManeuverCompletion(this, state);
   }
   // otherwise will auto-terminate when motion ends anyway
}


// XXX need to rework this end state stuff
void cFlexBowManeuver::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   sMcMoveState state;

   AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
//   m_pMotor->StopMotion(virtMotion); // XXX hopefully just stopping it on deletion will be enough
               // there's a weird glitching bug, where mp library things that
               // overlay hasn't ended, but base motion has.
   if(motionNum==m_MotionNum || motionNum==m_FlexBowMotion)
   {
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
   }
}
