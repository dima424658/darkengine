/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrgact.cpp,v 1.16 1999/08/05 17:09:13 Justin Exp $

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
#include <iobjsys.h>
#include <appagg.h>
#include <config.h>
#include <fix.h>
#include <textarch.h>

      // must be last header
#include <dbmem.h>

class cGroundActionManeuverFactory: public cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      

   virtual IManeuver *LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile);
};

typedef struct
{
   int m_SchemaID;
   float m_Duration;
   float m_TimeWarp;
   float m_Distance;
   float m_Stretch;
   ObjID m_FocusObj;
   fix m_TurnSpeed;
protected:
   BOOL m_DoRotate;
   mxs_angvec m_Direction;
   int m_MotionNum;
} sGroundActionData;

class cGroundActionManeuver: public cManeuver, public sGroundActionData
{
public:
   cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema);
   cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const char *name);
   cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const mxs_angvec& direction);
   cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const int motionNum, const int dummy);
   cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success);
   ~cGroundActionManeuver();

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
      ITagFile_Move(pTagFile,(char *)((sGroundActionData*)this),sizeof(*((sGroundActionData*)this)));
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

};

cGroundActionManeuverFactory g_GroundActionManeuverFactory;
EXTERN cManeuverFactory *g_pGroundActionManeuverFactory=&g_GroundActionManeuverFactory;


cMotionPlan *cGroundActionManeuverFactory::CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cGroundActionManeuver *pMnvr;

   Assert_(pSchema);

   if(params.mask&kMotParmFlag_Name)
   {
      pMnvr = new cGroundActionManeuver(pMotor, pCoord, (char *)params.name);
   } else if(params.mask&kMotParmFlag_MotNum)
   {
      pMnvr = new cGroundActionManeuver(pMotor, pCoord, params.motionNum,-1);
   } else
   {
      if(params.mask&kMotParmFlag_Facing)
      {
         pMnvr = new cGroundActionManeuver(pMotor, pCoord, pSchema, params.facing);
      } else
      {
         pMnvr = new cGroundActionManeuver(pMotor, pCoord, pSchema);
      }
   }
   pMnvr->m_SchemaID=pSchema->GetSchemaID();
   pMnvr->m_Duration=-1.0;
   pMnvr->m_TimeWarp=1.0;
   pMnvr->m_Distance=-1.0;
   pMnvr->m_Stretch=1.0;

   if(params.mask&kMotParmFlag_FocusObj)
   {
      pMnvr->m_FocusObj=params.focus;
   } else
   {
      pMnvr->m_FocusObj=OBJ_NULL;
   }
   pMnvr->m_TurnSpeed = params.turnspeed;
   
   if(!pSchema->GetDuration(&pMnvr->m_Duration))
      pSchema->GetTimeWarp(&pMnvr->m_TimeWarp);
   if(!pSchema->GetDistance(&pMnvr->m_Distance))
      pSchema->GetStretch(&pMnvr->m_Stretch);

   AssertMsg(pMnvr,"could not alloc maneuver");

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}


IManeuver *cGroundActionManeuverFactory::LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile)
{
   BOOL success;
   cGroundActionManeuver *pMnvr=new cGroundActionManeuver(pMotor,pCoord,pTagFile,&success);

   if(!success)
   {
      delete pMnvr;
      return NULL;
   }
   return pMnvr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// GroundAction MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define MAX_INTERESTED_MOTIONS 10

cGroundActionManeuver::cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   
   m_DoRotate=FALSE;
   m_SchemaID=pSchema->GetSchemaID();

   m_MotionNum=-1;
   if (IsRemoteGhost(pMotor->GetObjID()))
   {
      m_MotionNum = GetGhostMotionNumber();
#ifndef SHIP
      if (config_is_defined("ghost_motion_spew"))
      {
         mprintf("rGact: Remote ghost %d playing schema %d, motion %d\n", pMotor->GetObjID(), m_SchemaID, m_MotionNum);
      }
#endif
   }
   if (m_MotionNum==-1) // not a remoteghost, or remghost didnt want to deal 
   {
      if (!pSchema->GetRandomMotion(&m_MotionNum))
      {
         Warning(("rGact: CombatManeuver(False!): unable to find motion\n"));
         m_MotionNum=-1;
      }

      if (IsLocalGhost(pMotor->GetObjID()))
      {
#ifndef SHIP
         if (config_is_defined("ghost_motion_spew"))
         {
            mprintf("rGact: Local ghost %d informing remote about schema %d, motion %d\n", 
                    pMotor->GetObjID(), m_SchemaID, m_MotionNum);
         }
#endif
         GhostSendMoCap(pMotor->GetObjID(),m_SchemaID,m_MotionNum,FALSE);
      }
   }
}

cGroundActionManeuver::cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const char *name)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   
   m_DoRotate=FALSE;

   if (IsRemoteGhost(pMotor->GetObjID()))
      Warning(("illegal cGroundActionManeuver constructor for remote ghost %d\n", pMotor->GetObjID()));

   m_MotionNum=MotDescNameGetNum((char *)name);
   if(m_MotionNum==-1)
   {
      Warning(("GroundActionManeuver: unable to find motion named %s\n",(char *)name));
   }
}

cGroundActionManeuver::cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const int motionNum, const int dummy)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;   
   m_DoRotate=FALSE;

   if (IsRemoteGhost(pMotor->GetObjID()))
      Warning(("illegal cGroundActionManeuver constructor for remote ghost %d\n", pMotor->GetObjID()));

   m_MotionNum=motionNum;
   
   if(m_MotionNum==-1)
      Warning(("GroundActionManeuver: unable to find motion\n"));
}

cGroundActionManeuver::cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const mxs_angvec& direction)
{
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_DoRotate=TRUE;
   m_Direction=direction;

   if (IsRemoteGhost(pMotor->GetObjID()))
      Warning(("illegal cGroundActionManeuver constructor for remote ghost %d\n", pMotor->GetObjID()));

   Assert_(pSchema);
   if(!pSchema->GetRandomMotion(&m_MotionNum))
   {
      Warning(("CombatManeuver: unable to find motion\n"));
      m_MotionNum=-1;
   }
}

cGroundActionManeuver::cGroundActionManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success)
{
   *success=TRUE;
   ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
   ITagFile_Move(pTagFile,(char *)((sGroundActionData*)this),sizeof(*((sGroundActionData*)this)));
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   Assert_(m_pMotor);

   if(m_pMotor&&m_MotionNum!=-1)
   {
      m_MotionNum=m_pMotor->SwizzleMotion(m_MotionNum);   
      if(m_MotionNum==-1) // couldn't swizzle
         *success=FALSE;
   }

   if (IsRemoteGhost(pMotor->GetObjID()))
      Warning(("illegal cGroundActionManeuver constructor for remote ghost %d\n", pMotor->GetObjID()));

   // check that motor is in same state we think we are
   if(m_pMotor->GetCurrentMotion()!=m_MotionNum)   // out of synch!
   {
      Warning(("cGroundActionManuever: cannot load maneuver because creature is out of synch\n"));
      m_MotionNum=-1;
      *success=FALSE;
   }

   // swizzle focus obj
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   m_FocusObj=pObjSys->RemapOnLoad(m_FocusObj);
}

cGroundActionManeuver::~cGroundActionManeuver()
{
   if(!m_pMotor)
      return;

   // Stop motion if one is playing

   if(m_MotionNum>=0)
   {
      m_pMotor->StopMotion(m_MotionNum);

      // notify scripting system
      MvrSendMotionMessage(m_pMotor->GetObjID(),m_MotionNum,kMotionEnd,NULL);
   }
   if(m_FocusObj!=OBJ_NULL)
   {
      m_pMotor->SetFocus(OBJ_NULL,m_TurnSpeed);
   }
}

void cGroundActionManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   AssertMsg(FALSE,"GetExpectedEndMoveState not implemented for GroundAction maneuver");
}

void cGroundActionManeuver::Execute()
{
   Assert_(m_pMotor);
   Assert_(m_pCoord);   
   m_pCoord->ClearInterMnvrState();
   sMcMotorState *pS=m_pCoord->GetInterMnvrState();
   pS->lastSchemaID=m_SchemaID;

   if(m_FocusObj!=OBJ_NULL)
   {
      m_pMotor->SetFocus(m_FocusObj,m_TurnSpeed);
   }

   if(m_MotionNum<0)
   {
      sMcMoveState state;
      // XXX need to rework this end state stuff

      if(m_DoRotate)
      {
         m_pMotor->SetOrientation(&m_Direction);
      }
      AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
   } else
   {
      mxs_real groundHeight;
      ObjID    standingObj;
      Location *curLoc=(Location *)m_pMotor->GetLocation();

      Assert_(g_pMotionSet);

      // keep grounded
      if(MvrFindGroundHeight(m_pMotor->GetObjID(),curLoc,&groundHeight,&standingObj))
      {
         float desiredZ=0;
         mxs_vector buttPos;

         MvrSetStandingObj(m_pMotor->GetObjID(), standingObj);

         MvrGetEndButtHeight(m_pMotor,m_MotionNum,&desiredZ);
         desiredZ+=groundHeight;

         // and set its offset
         if (!IsTextureObj(standingObj))
         {
            mxs_vector offset = {curLoc->vec.x, curLoc->vec.y, desiredZ};
            MvrSetStandingOffset(m_pMotor->GetObjID(), standingObj, &offset);
         }
         else
         {
            mxs_vector offset = {0, 0, groundHeight};
            MvrSetTerrStandingOffset(m_pMotor->GetObjID(), &offset);
         }

         mx_copy_vec(&buttPos,&curLoc->vec);
         buttPos.z=desiredZ;

         if(m_DoRotate)
         {
            // NOTE: this assumes that direction of action in motion capture
            // is along the x-axis
            m_pMotor->SetTransform(&buttPos,&m_Direction);
         } else
         {
            m_pMotor->SetPosition(&buttPos);
         }
      } else
      {
         if(m_DoRotate)
         {
            // NOTE: this assumes that direction of action in motion capture
            // is along the x-axis
            m_pMotor->SetOrientation(&m_Direction);
         } 
      }

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

void cGroundActionManeuver::NotifyAboutFrameFlags(const int flags)
{
   if(m_pMotor)
   {
      MvrProcessStandardFlags(m_pMotor,m_pCoord,flags);
      MvrSendMotionMessage(m_pMotor->GetObjID(), m_MotionNum,kMotionFlagReached,flags);
   }
}

// XXX need to rework this end state stuff
void cGroundActionManeuver::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   sMcMoveState state;

   AssertMsg(m_pCoord,"No motion coordinator for maneuver!");

   // notify scripting system
   if(m_pMotor)
      MvrSendMotionMessage(m_pMotor->GetObjID(),m_MotionNum,kMotionEnd,NULL);

   m_MotionNum=-1;
   m_pCoord->NotifyAboutManeuverCompletion(this,state);
}
