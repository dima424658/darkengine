/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrglide.cpp,v 1.3 1997/12/19 15:28:32 TOML Exp $

#include <mmanuver.h>
#include <mcoord.h>
#include <wrtype.h>
#include <portal.h>
#include <mvrutils.h>

      // must be last header
#include <dbmem.h>

class cGlideManeuverFactory: public cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const tMSkillHandle skill,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      
};

class cGlideManeuver: public cManeuver
{
public:
   ~cGlideManeuver() {}
   cGlideManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_angvec& direction, const mxs_real speed, const ulong duration);

   virtual void GetExpectedEndMoveState(sMcMoveState& moveState);
   virtual void Execute();
private:
   mxs_vector m_EndPos;
   mxs_angvec m_EndFac;
};

cGlideManeuverFactory g_GlideManeuverFactory;
EXTERN cManeuverFactory *g_pGlideManeuverFactory=&g_GlideManeuverFactory;


cMotionPlan *cGlideManeuverFactory::CreatePlan(const tMSkillHandle skill,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cGlideManeuver *pMnvr;
   sMGaitSkillData *pGait=MSkillGetGaitSkillData(skill);

   if(!(params.mask&kMotParmFlag_Duration) || !(params.mask&kMotParmFlag_Direction))
      return NULL;

   if(params.mask&kMotParmFlag_ExactSpeed)
      pMnvr = new cGlideManeuver(pMotor, pCoord, params.direction, params.exactSpeed, params.duration);
   else
      pMnvr = new cGlideManeuver(pMotor, pCoord, params.direction, (pGait->minVel+pGait->maxVel)/2, params.duration);

   AssertMsg(pMnvr,"could not alloc maneuver");

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// Glide MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

cGlideManeuver::cGlideManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const mxs_angvec& direction, const mxs_real speed, const ulong duration)
{
   mxs_vector delta;
   mxs_matrix mat;
   float frac;
   mxs_real groundHeight;

   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_EndFac=direction;

   const Location *loc=pMotor->GetLocation();
   mx_ang2mat(&mat,(mxs_angvec *)&direction);
   mx_unit_vec(&delta,0);
   mx_mat_muleq_vec(&mat,&delta);
   // duration is in msec.
   frac=speed*((float)duration)/1000;
   // speed and duration
   mx_scaleeq_vec(&delta,frac);

   mx_add_vec(&m_EndPos,(mxs_vector *)&loc->vec,&delta);
   if(!MvrFindGroundHeight(loc,&groundHeight))
      groundHeight=0;
   m_EndPos.z=groundHeight+pMotor->GetButtZOffset();
}

void cGlideManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   moveState.position=m_EndPos;
   moveState.facing=m_EndFac;
   moveState.speed=0;
   moveState.direction=m_EndFac;
}

void cGlideManeuver::Execute()
{
   Location toLoc,hitLoc;
   sMcMoveState moveState;

   const Location *fromLoc=m_pMotor->GetLocation();

   MakeHintedLocation(&toLoc,m_EndPos.x,m_EndPos.y,m_EndPos.z,(Location *)fromLoc);
   if(PortalRaycast((Location *)fromLoc,&toLoc,&hitLoc,TRUE))
   {
      // safe to update
      m_pMotor->SetTransform(&m_EndPos,&m_EndFac);
      GetExpectedEndMoveState(moveState);
   } else
   {
      m_pMotor->SetTransform(&fromLoc->vec,&m_EndFac);
      moveState.position=fromLoc->vec;
      moveState.facing=m_EndFac;
      moveState.direction=m_EndFac;
      moveState.speed=0;
   }
   m_pCoord->NotifyAboutManeuverCompletion(this,moveState);
}
