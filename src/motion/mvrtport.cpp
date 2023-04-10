/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrtport.cpp,v 1.2 1997/12/19 15:28:33 TOML Exp $

#include <mmanuver.h>
#include <mcoord.h>

      // must be last header
#include <dbmem.h>

class cTeleportManeuverFactory: public cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const tMSkillHandle skill,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      
};

class cTeleportManeuver: public cManeuver
{
public:
   cTeleportManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_vector& position);
   cTeleportManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_vector& position, const mxs_angvec& facing);
   cTeleportManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_angvec& facing);
   virtual ~cTeleportManeuver() {}

   virtual void GetExpectedEndMoveState(sMcMoveState& moveState);
   virtual void Execute();
private:
   mxs_vector m_EndPos;
   mxs_angvec m_EndFac;
};

cTeleportManeuverFactory g_TeleportManeuverFactory;
EXTERN cManeuverFactory *g_pTeleportManeuverFactory=&g_TeleportManeuverFactory;


cMotionPlan *cTeleportManeuverFactory::CreatePlan(const tMSkillHandle skill,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cTeleportManeuver *pMnvr;

   if(params.mask&kMotParmFlag_Facing)
      if(params.mask&kMotParmFlag_Position)
         pMnvr = new cTeleportManeuver(pMotor, pCoord, params.position, params.facing);
      else
         pMnvr = new cTeleportManeuver(pMotor, pCoord, params.facing);
   else if(params.mask&kMotParmFlag_Facing)
      pMnvr = new cTeleportManeuver(pMotor, pCoord, params.position);
   else
      return NULL;

   AssertMsg(pMnvr,"could not alloc maneuver");

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// TELEPORT MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

cTeleportManeuver::cTeleportManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_vector& position, const mxs_angvec& facing)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_EndPos=position;
   m_EndFac=facing;
}

cTeleportManeuver::cTeleportManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_vector& position)
{
   mxs_vector opos;
   mxs_angvec ofac;

   pMotor->GetTransform(&opos,&ofac);

   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_EndPos=position;
   m_EndFac=ofac;
}

cTeleportManeuver::cTeleportManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_angvec& facing)
{
   mxs_vector opos;
   mxs_angvec ofac;

   pMotor->GetTransform(&opos,&ofac);

   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_EndPos=opos;
   m_EndFac=facing;
}

void cTeleportManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   moveState.position=m_EndPos;
   moveState.facing=m_EndFac;
   moveState.speed=0;
   moveState.direction=m_EndFac;
}

void cTeleportManeuver::Execute()
{
   sMcMoveState moveState;

   m_pMotor->SetTransform(&m_EndPos,&m_EndFac);
   GetExpectedEndMoveState(moveState);
   m_pCoord->NotifyAboutManeuverCompletion(this,moveState);
}
