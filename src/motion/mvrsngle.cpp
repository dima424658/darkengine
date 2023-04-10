/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrsngle.cpp,v 1.22 1999/08/05 17:10:46 Justin Exp $

#include <config.h>
#include <mmanuver.h>
#include <mcoord.h>
#include <wrtype.h>
#include <portal.h>
#include <mvrutils.h>
#include <motdesc.h>
#include <mclntapi.h>
#include <motschem.h>
#include <motmngr.h>
#include <ghostmvr.h>

      // must be last header
#include <dbmem.h>

class cSingleManeuverFactory: public cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      

   virtual IManeuver *LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile);
};

typedef struct
{
   BOOL m_DoRotate;
   mxs_angvec m_Direction;
   int m_MotionNum;
   float m_Duration;
   float m_TimeWarp;
   float m_Distance;
   float m_Stretch;
} sSingleData;

class cSingleManeuver: public cManeuver, public sSingleData
{
public:
   cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema);
   cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const char *name);
   cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const mxs_angvec& direction);
   cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const int motionNum, const int dummy);
   cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success);
   ~cSingleManeuver();

////////
// Motion System Client Functions
////////
   virtual void GetExpectedEndMoveState(sMcMoveState& moveState);

////////
// Motion Coordinator Functions
////////
   virtual void Execute();
   virtual void Save(ITagFile *pTagFile) { 
      ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
      ITagFile_Move(pTagFile,(char *)((sSingleData*)this),sizeof(*((sSingleData*)this)));
   }

////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect() {}
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel) \
      { MvrCalcSlidingCollisionResponse(pForces,nForces,pCurrentVel,pNewVel); }
   virtual void NotifyAboutBeingStuck() { m_pMotor->StopMotion(m_MotionNum); }

#if 0
//
// Motion status notification functions
//
   virtual void NotifyAboutFrameUpdateBegin(const ObjID);
   virtual void NotifyAboutFrameUpdateEnd(const ObjID);
   virtual void NotifyAboutFrameFlags(const ObjID, const int);
#endif

   virtual void NotifyAboutFrameFlags(const int);
   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags);

private:
};

cSingleManeuverFactory g_SingleManeuverFactory;
EXTERN cManeuverFactory *g_pSingleManeuverFactory=&g_SingleManeuverFactory;


cMotionPlan *cSingleManeuverFactory::CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cSingleManeuver *pMnvr;

   if(params.mask&kMotParmFlag_Name)
   {
      pMnvr = new cSingleManeuver(pMotor, pCoord, (char *)params.name);
   } else if(params.mask&kMotParmFlag_MotNum)
   {
      pMnvr = new cSingleManeuver(pMotor, pCoord, params.motionNum,-1);
   } else
   {
      if(params.mask&kMotParmFlag_Facing)
      {
         pMnvr = new cSingleManeuver(pMotor, pCoord, pSchema, params.facing);
      } else
      {
         pMnvr = new cSingleManeuver(pMotor, pCoord, pSchema);
      }
   }
   AssertMsg(pMnvr,"could not alloc maneuver");

   pMnvr->m_Duration=-1.0;
   pMnvr->m_TimeWarp=1.0;
   pMnvr->m_Distance=-1.0;
   pMnvr->m_Stretch=1.0;
   if(!pSchema->GetDuration(&pMnvr->m_Duration))
      pSchema->GetTimeWarp(&pMnvr->m_TimeWarp);
   if(!pSchema->GetDistance(&pMnvr->m_Distance))
      pSchema->GetStretch(&pMnvr->m_Stretch);

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}

IManeuver *cSingleManeuverFactory::LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile)
{
   BOOL success;
   cSingleManeuver *pMnvr=new cSingleManeuver(pMotor,pCoord,pTagFile,&success);

   if(!success)
   {
      delete pMnvr;
      return NULL;
   }
   return pMnvr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// Single MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define MAX_INTERESTED_MOTIONS 10

cSingleManeuver::cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   
   m_DoRotate=FALSE;

   m_MotionNum=-1;
   if (IsRemoteGhost(pMotor->GetObjID()))
   {
      m_MotionNum = GetGhostMotionNumber();

#ifndef SHIP
      if (config_is_defined("ghost_motion_spew"))
      {
         mprintf("rSngle: Remote ghost %d playing schema %d, motion %d\n", 
                 pMotor->GetObjID(), pSchema->GetSchemaID(), m_MotionNum);
      }
#endif
   }
   if (m_MotionNum==-1)  // not a remoteghost, or remghost didnt want to deal 
   {
      Assert_(pSchema);
      if(!pSchema->GetRandomMotion(&m_MotionNum))
      {
         Warning(("rSngle: CombatManeuver(Liar!): unable to find motion\n"));
         m_MotionNum=-1;
      }

      if (IsLocalGhost(pMotor->GetObjID()))
      {
#ifndef SHIP
         if (config_is_defined("ghost_motion_spew"))
         {
            mprintf("rSngle: Local ghost %d informing remote about schema %d, motion %d\n", 
                    pMotor->GetObjID(), pSchema->GetSchemaID(), m_MotionNum);
         }
#endif
         // @TODO: CAC: tell remote ghost that we're playing a motion
         GhostSendMoCap(pMotor->GetObjID(),pSchema->GetSchemaID(),m_MotionNum,FALSE);
      }
   }
}

cSingleManeuver::cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const char *name)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   
   m_DoRotate=FALSE;

   if (IsRemoteGhost(pMotor->GetObjID()))
      Warning(("illegal cSingleManeuver constructor for remote ghost %d\n", pMotor->GetObjID()));

   m_MotionNum=MotDescNameGetNum((char *)name);
   if(m_MotionNum==-1)
   {
      Warning(("SingleManeuver: unable to find motion named %s\n",(char *)name));
   }
}

cSingleManeuver::cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const int motionNum, const int dummy)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   
   m_DoRotate=FALSE;

   m_MotionNum=motionNum;
   
   if(m_MotionNum==-1)
      Warning(("SingleManeuver: unable to find motion\n"));
}

cSingleManeuver::cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const mxs_angvec& direction)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_DoRotate=TRUE;
   m_Direction=direction;

   Assert_(pSchema);
   if(!pSchema->GetRandomMotion(&m_MotionNum))
   {
      Warning(("CombatManeuver: unable to find motion\n"));
      m_MotionNum=-1;
   }
}

cSingleManeuver::cSingleManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success)
{
   *success=TRUE;
   ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
   ITagFile_Move(pTagFile,(char *)((sSingleData*)this),sizeof(*((sSingleData*)this)));
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   Assert_(m_pMotor);
   if(m_pMotor&&m_MotionNum!=-1)
   {
      m_MotionNum=m_pMotor->SwizzleMotion(m_MotionNum);   
      if(m_MotionNum==-1) // couldn't swizzle
         *success=FALSE;
   }
   // check that motor is in same state we think we are
   if(m_pMotor->GetCurrentMotion()!=m_MotionNum)   // out of synch!
   {
      Warning(("cSingleManuever: cannot load maneuver because creature is out of synch\n"));
      m_MotionNum=-1;
      *success=FALSE;
   }
}

cSingleManeuver::~cSingleManeuver()
{
   // Stop motion if one is playing

   if(m_MotionNum>=0 && m_pMotor)
   {
      m_pMotor->StopMotion(m_MotionNum);

      // notify scripting system
      MvrSendMotionMessage(m_pMotor->GetObjID(),m_MotionNum,kMotionEnd,NULL);
   }
}

void cSingleManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   AssertMsg(FALSE,"GetExpectedEndMoveState not implemented for single maneuver");
}

void cSingleManeuver::Execute()
{
   if(m_DoRotate)
   {
      // NOTE: this assumes that direction of action in motion capture
      // is along the x-axis

      m_pMotor->SetOrientation(&m_Direction);
   }
   if(m_MotionNum<0)
   {
      sMcMoveState state;
      // XXX need to rework this end state stuff

      AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
   } else
   {
      if(m_pMotor)
      {
         mps_motion_param param;

         param.flags=0;

         if(m_Duration>=0)
         {
            param.flags|=MP_FIXED_DURATION;
            param.fixed_duration=m_Duration;
         } else if(m_TimeWarp!=1.0)
         {
            param.flags|=MP_DURATION_SCALE;
            param.duration_scalar=m_TimeWarp;
         }
         if(m_Distance>=0)
         {
            mxs_vector xlat;
            mxs_real mag;

            g_pMotionSet->GetTranslation(m_MotionNum,&xlat);
            mag=mx_mag_vec(&xlat);
            if(mag>0.1)
            {
               param.flags|=MP_STRETCH;
               param.stretch=m_Distance/mag;
            }
         } else if(m_Stretch!=1.0)
         {
            param.flags|=MP_STRETCH;
            param.stretch=m_Stretch;
         }
         m_pMotor->StartMotionWithParam(m_MotionNum,&param);
         // notify scripting system
         MvrSendMotionMessage(m_pMotor->GetObjID(),m_MotionNum,kMotionStart,NULL);
      }
   }
}

void cSingleManeuver::NotifyAboutFrameFlags(const int flags)
{
   if(m_pMotor)
   {
      MvrProcessStandardFlags(m_pMotor,m_pCoord,flags);
      MvrSendMotionMessage(m_pMotor->GetObjID(), m_MotionNum,kMotionFlagReached,flags);
   }
}

// XXX need to rework this end state stuff
void cSingleManeuver::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   sMcMoveState state;

   AssertMsg(m_pCoord,"No motion coordinator for maneuver!");

   // notify scripting system
   if(m_pMotor)
      MvrSendMotionMessage(m_pMotor->GetObjID(),m_MotionNum,kMotionEnd,NULL);

   m_MotionNum=-1;
   m_pCoord->NotifyAboutManeuverCompletion(this,state);
}
