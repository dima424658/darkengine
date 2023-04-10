/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrcmbat.cpp,v 1.31 1999/12/09 20:19:31 BODISAFA Exp $

#include <mmanuver.h>
#include <mcoord.h>
#include <wrtype.h>
#include <portal.h>
#include <mvrutils.h>
#include <motdesc.h>
#include <mclntapi.h>
#include <mprintf.h>
#include <weapon.h>
#include <osystype.h>
#include <cfgdbg.h>
#include <mschutil.h>
#include <weaphalo.h>
#include <mvrflags.h>
#include <mschbase.h>
#include <appagg.h>
#include <iobjsys.h> // woo hoo comm-y stuff
#include <ghostmvr.h>
#include <textarch.h>
#include <crwpnapi.h>

      // must be last header
#include <dbmem.h>

// each motor controller that wants to look at inter-maneuver state should
// have unique value for lastControllerID field.
#define kMvrUID_Combat 0x1 

// inter-maneuver state flag
#define kMnvrState_HackBlockThing 0x1

class cCombatManeuverFactory: public cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const cMotionSchema *schema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      

   virtual IManeuver *LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile);
};

typedef struct
{
   BOOL m_DoRotate;
   mxs_angvec m_Direction;
   int m_MotionNum;
   int m_RestMotion;
   ObjID m_WeaponObj;
   BOOL m_ReturningToRest;
   BOOL m_DoingSwing;
   int m_SchemaID;
   float m_TimeWarp;
   int m_ActionType;
   ObjID m_FocusObj;
   BOOL m_IsPhysical;
   fix m_TurnSpeed;
} sCombatData;

class cCombatManeuver: public cManeuver, private sCombatData
{
public:
   cCombatManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams *pParams, const mxs_angvec *pDirection=NULL,BOOL doDir=FALSE);
   cCombatManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success);
   ~cCombatManeuver();

////////
// Motion System Client Functions
////////
   virtual void GetExpectedEndMoveState(sMcMoveState& moveState);

////////
// Motion Coordinator Functions
////////
   virtual void Execute();
   virtual void Finish();
   virtual void SetMotor(IMotor *pMotor);
   virtual void Save(ITagFile *pTagFile);
   virtual void DestroyWeapon();


////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect() {}
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel) \
      { MvrCalcSlidingCollisionResponse(pForces,nForces,pCurrentVel,pNewVel); }
   virtual void NotifyAboutBeingStuck() { m_pMotor->StopMotion(m_MotionNum); }
   virtual void NotifyAboutMotionAbortRequest();

//
// Motion status notification functions
//
   virtual void NotifyAboutFrameFlags(const int);
#if 0
   virtual void NotifyAboutFrameUpdateBegin(const ObjID);
   virtual void NotifyAboutFrameUpdateEnd(const ObjID);
#endif

   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags);

private:
};

cCombatManeuverFactory g_CombatManeuverFactory;
EXTERN cManeuverFactory *g_pCombatManeuverFactory=&g_CombatManeuverFactory;

cMotionPlan *cCombatManeuverFactory::CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cCombatManeuver *pMnvr;

   if(params.mask&kMotParmFlag_Facing)
   {
      pMnvr = new cCombatManeuver(pMotor, pCoord, pSchema, &params,&params.facing,TRUE);
   } else
   {
      pMnvr = new cCombatManeuver(pMotor, pCoord, pSchema, &params);
   }
   AssertMsg(pMnvr,"could not alloc maneuver");

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}

IManeuver *cCombatManeuverFactory::LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile)
{
   BOOL success;
   cCombatManeuver *pMnvr=new cCombatManeuver(pMotor,pCoord,pTagFile,&success);

   if(!success)
   {
      delete pMnvr;
      return NULL;
   }
   return pMnvr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// Combat MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define MAX_INTERESTED_MOTIONS 10


cCombatManeuver::cCombatManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams *pParams, const mxs_angvec *pDirection, BOOL doDir)
{
   Assert_(pSchema);
   Assert_(pMotor);

   if(doDir)
   {
      m_DoRotate=TRUE;
      m_Direction=*pDirection;
   } else
   {
      m_DoRotate=FALSE;
   }
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_ReturningToRest=FALSE;
   m_DoingSwing=FALSE;
   m_WeaponObj=OBJ_NULL;
   m_SchemaID=pSchema->GetSchemaID();
   if(!MSchUGetSwordActionType(pSchema,&m_ActionType))
      m_ActionType=kMSAT_Swing; // default
   m_IsPhysical=FALSE;

   if(pParams->mask&kMotParmFlag_FocusObj)
      m_FocusObj=pParams->focus;
   else
      m_FocusObj=OBJ_NULL;
   m_TurnSpeed = pParams->turnspeed;
   
   int startMotOff;

   if (m_ActionType == kMSAT_Swing)
   {
      if (!pSchema->GetMotion(0,&m_RestMotion))
      {
         Warning(("CombatManeuver: no rest position\n"));
         m_RestMotion=-1;
      }
      startMotOff=1;
   } 
   else
   if (m_ActionType == kMSAT_GeneralBlock)
   {
      if (!pSchema->GetMotion(0,&m_RestMotion))
      {
         Warning(("CombatManeuver: no rest position\n"));
         m_RestMotion=-1;
      }
      startMotOff=0;
   }
   else
   {
      m_RestMotion=-1;
      startMotOff=0;
   } 

   m_MotionNum=-1;
   if (IsRemoteGhost(pMotor->GetObjID()))
   {
      m_MotionNum = GetGhostMotionNumber();
#ifndef SHIP
      if (config_is_defined("ghost_motion_spew"))
      {
         mprintf("rCmbat: Remote ghost %d playing schema %d, motion %d\n", pMotor->GetObjID(), m_SchemaID, m_MotionNum);
      }
#endif
   }
   if (m_MotionNum==-1)  // not a remoteghost, or remghost didnt want to deal 
   {
      if(!pSchema->GetRandomMotionInRange(startMotOff,pSchema->NumMotions()-1,&m_MotionNum))
      {
         Warning(("rCmbat: CombatManeuver(True!): unable to find motion\n"));
         m_MotionNum=-1;
      }
      
      if (IsLocalGhost(pMotor->GetObjID()))
      {
#ifndef SHIP
         if (config_is_defined("ghost_motion_spew"))
         {
            mprintf("rCmbat: Local ghost %d informing remote about schema %d, motion %d\n", 
                    pMotor->GetObjID(), m_SchemaID, m_MotionNum);
         }
#endif
         // @TODO: CAC: tell remote ghost that we're playing a motion
         GhostSendMoCap(pMotor->GetObjID(),m_SchemaID,m_MotionNum,FALSE);
      }
   }

   m_TimeWarp=1.0;
   pSchema->GetTimeWarp(&m_TimeWarp);
}

cCombatManeuver::cCombatManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success)
{
   *success=TRUE;
   ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
   ITagFile_Move(pTagFile,(char *)((sCombatData *)this),sizeof(*((sCombatData*)this)));
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   Assert_(m_pMotor);
   // swizzle motion number
   if(m_MotionNum!=-1)
   {
      m_MotionNum=m_pMotor->SwizzleMotion(m_MotionNum);   
      if(m_MotionNum==-1) // couldn't swizzle
         *success=FALSE;
   }
   // check that motor is in same state we think we are
   if(m_pMotor->GetCurrentMotion()!=m_MotionNum)   // out of synch!
   {
      Warning(("cGroundLocoManuever: cannot load maneuver because creature is out of synch\n"));
      m_MotionNum=-1;
      *success=FALSE;
   }
   // swizzle objects and other mots
   // @NOTE: not swizzling rest motion, because this wasn't 
   // necessarily playing when saved, so won't swizzle correctly.
   // swizzling only needed when motion db changes KJ 10/98
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   m_WeaponObj=pObjSys->RemapOnLoad(m_WeaponObj);
   m_FocusObj=pObjSys->RemapOnLoad(m_FocusObj);
}

void cCombatManeuver::Save(ITagFile *pTagFile)
{
   // write app_data from cmaneuver
   ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
   ITagFile_Move(pTagFile,(char *)((sCombatData*)this),sizeof(*((sCombatData*)this)));
}

cCombatManeuver::~cCombatManeuver()
{
   // Stop motion if one is playing

   if(m_pMotor)
   {
      if(m_MotionNum>=0)
         m_pMotor->StopMotion(m_MotionNum);
      m_pMotor->SetFocus(OBJ_NULL,m_TurnSpeed);
   }
   DestroyWeapon();
}

void cCombatManeuver::SetMotor(IMotor *pMotor) 
{ 
   DestroyWeapon();
   m_pMotor=pMotor;
}

void cCombatManeuver::DestroyWeapon()
{
   if(m_WeaponObj!=OBJ_NULL)
   {
      ObjID me;
      // detach weapon
      if(!m_pMotor)
      {
         Warning(("detaching weapon %d from non-existant creature!\n",m_WeaponObj));
         me=OBJ_NULL;
      } else
      {
         me=m_pMotor->GetObjID();
      }

      // in case never dephysicalized
      if(m_IsPhysical)
      {
         MakeAIWeaponNonPhysical(me,m_WeaponObj); // in case not already done
         if(m_ActionType==kMSAT_GeneralBlock)
            WeaponHaloDeflate(me,m_WeaponObj); // @NOTE: this may get called twice
         ConfigSpew("MnvrTrace",("%d: de-physicalizing\n",me));
      }
      ConfigSpew("MnvrTrace",("%d: unequip weapon\n",me));
      UnEquipAIWeapon(me,m_WeaponObj);
      m_WeaponObj=NULL;
   }
}


void cCombatManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   AssertMsg(FALSE,"GetExpectedEndMoveState not implemented for Combat maneuver");
}

static float g_HackTestPercent = 0.1;

void cCombatManeuver::Execute()
{
// calculate end height of motion
   mxs_real height;
   ObjID    standing_obj;
   int startFrame=0;
   ulong motFlags=NULL;
   BOOL doingBlockContinue=FALSE;

   AssertMsg(m_pMotor,"no motor for combat maneuver\n");
   Assert_(m_pCoord);   

   sMcMotorState *pS=m_pCoord->GetInterMnvrState();
   // check if last maneuver said to do block thing  
   if(m_ActionType==kMSAT_DirectedBlock && pS->lastControllerID==kMvrUID_Combat && pS->flags&kMnvrState_HackBlockThing)
   {
      doingBlockContinue=TRUE;
      // @TODO: find out how far through to start motion.
      // @DIPPY: this should be a "getmotioninfo" function elsewhere
      if(m_MotionNum>=0)
      {
         startFrame=mp_motion_list[m_MotionNum].info.num_frames*pS->lastMotPercentCompleted;
         ConfigSpew("MnvrTrace",("doing continue mot %d\n",m_MotionNum));
         motFlags=kMotStartFlag_SetStartFrame;
      }
   } 

   m_pCoord->ClearInterMnvrState();
   pS->lastSchemaID=m_SchemaID;
   pS->lastControllerID=kMvrUID_Combat;

   Location *pLocation=(Location *)m_pMotor->GetLocation();

   if(m_FocusObj!=OBJ_NULL)
   {
      m_pMotor->SetFocus(m_FocusObj,m_TurnSpeed);
   }

   MvrFindGroundHeight(m_pMotor->GetObjID(),pLocation,&height,&standing_obj);
   mxs_vector buttPos;

   MvrSetStandingObj(m_pMotor->GetObjID(), standing_obj);

   mx_copy_vec(&buttPos,&pLocation->vec);
   if(!MvrGetEndButtHeight(m_pMotor,m_MotionNum,&buttPos.z))
      buttPos.z=0;
   buttPos.z+=height;

   // and set its offset
   if (!IsTextureObj(standing_obj))
      MvrSetStandingOffset(m_pMotor->GetObjID(), standing_obj, &buttPos);
   else
   {
      mxs_vector offset = {0, 0, height};
      MvrSetTerrStandingOffset(m_pMotor->GetObjID(), &offset);
   }

//   buttPos.z=height+m_pMotor->GetButtZOffset();

   if(m_DoRotate)
   {
      // NOTE: this assumes that direction of action in motion capture
      // is along the x-axis
      m_pMotor->SetTransform(&buttPos,&m_Direction);
   } else
   {
      m_pMotor->SetPosition(&buttPos);
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
         ObjID me=m_pMotor->GetObjID();

         // @TODO:  Bodisafa 12/1/1999
         // mode hould be related to AI maneuver.
         if(!EquipAIWeapon(me, &m_WeaponObj, kWeaponModeMelee))
         {
            Warning(("Could not attach weapon to %d\n",me));
            m_WeaponObj=OBJ_NULL;
         } 
         mps_motion_param param;

         param.flags=0;

         if(m_TimeWarp!=1.0)
         {
            param.flags|=MP_DURATION_SCALE;
            param.duration_scalar=m_TimeWarp;
         }
         m_pMotor->StartMotionWithParam(m_MotionNum,&param,motFlags,startFrame);
         m_DoingSwing=TRUE;

         if(doingBlockContinue)
         {
            // @HACK
            // to physicalize, since this won't get triggered since start frame is
            // offset
            NotifyAboutFrameFlags(MF_TRIGGER2);
         }
      }
   }
}

void cCombatManeuver::Finish()
{
   if(m_ActionType!=kMSAT_GeneralBlock)
      return;

   if(m_pMotor)
   {
      ConfigSpew("MnvrTrace",("%d: told to finish\n",m_pMotor->GetObjID()));
   }

   // store off how far through motion you are etc
   // if blocking, set secret maneuver state so next one will know to 
   // start part-way through.
   // @TODO: calculate how far through to start next motion
   sMcMotorState *pMS=m_pCoord->GetInterMnvrState();
   pMS->flags|=kMnvrState_HackBlockThing;
   pMS->lastMotPercentCompleted=g_HackTestPercent;
}

// XXX need to rework this end state stuff
void cCombatManeuver::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   sMcMoveState state;

   AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
   
   // check that it isn't just the swing aborting 
   if(m_ReturningToRest&&m_MotionNum!=motionNum)
      return;

   if(m_WeaponObj!=OBJ_NULL)
   {
      // detach weapon
      if(!m_pMotor)
      {
         Warning(("Cannot detach weapon %d from creature!\n",m_WeaponObj));
      } else
      {
         ObjID me=m_pMotor->GetObjID();

         ConfigSpew("MnvrTrace",("%d: finished swing\n",me));
         if(m_IsPhysical)
         {
            ConfigSpew("MnvrTrace",("%d: de-physicalizing\n",me));
            MakeAIWeaponNonPhysical(me,m_WeaponObj);
            // @TODO: make this happen on frame callback
            if(m_ActionType==kMSAT_GeneralBlock)
               WeaponHaloDeflate(me,m_WeaponObj);
            m_IsPhysical=FALSE;
         }
      }
   }

   m_pCoord->NotifyAboutManeuverCompletion(this,state);
}

// XXX need to rework this end state stuff
void cCombatManeuver::NotifyAboutMotionAbortRequest()
{
   if (!m_DoingSwing)
      return;

   Assert_(m_pMotor);

   // check that not already returning to rest
   if(m_ReturningToRest)
   {
      return;
   }

   // make weapon non-physical
   if(m_WeaponObj!=OBJ_NULL)
   {
      // detach weapon
      if(!m_pMotor)
      {
         Warning(("Cannot detach weapon %d from creature!\n",m_WeaponObj));
      } else
      {
         ObjID me=m_pMotor->GetObjID();

         ConfigSpew("MnvrTrace",("%d: de-physicalizing and un-equipping\n",me));
         MakeAIWeaponNonPhysical(me,m_WeaponObj);
//         UnEquipAIWeapon(me,m_WeaponObj);
         m_IsPhysical=FALSE;
      }
      m_WeaponObj=OBJ_NULL;
   }

   m_ReturningToRest=TRUE;

   // if have recover motion, stop motion and
   // start rest motion, so creature will "recover" to rest
   if(m_MotionNum>=0 && m_pMotor && m_RestMotion>=0)
   {
      int curMot;

      ConfigSpew("MnvrTrace",("%d: aborting\n",m_pMotor->GetObjID()));

      curMot=m_MotionNum;
      m_MotionNum=m_RestMotion;
      m_pMotor->StopMotion(curMot);
      m_DoingSwing=FALSE;

      if(m_MotionNum<0)
      {
         sMcMoveState state;
         
         m_pCoord->NotifyAboutManeuverCompletion(this,state);
         return;
      } 
      // make recovery be grounded
      //

      mxs_real height;
      ObjID    standing_obj;
   
      // find ground height
      Location *pLocation=(Location *)m_pMotor->GetLocation();
      MvrFindGroundHeight(m_pMotor->GetObjID(),pLocation,&height,&standing_obj);

      MvrSetStandingObj(m_pMotor->GetObjID(), standing_obj);

      // find butt translation at end of motion
      mxs_vector buttPos;
      mx_copy_vec(&buttPos,&pLocation->vec);
      if(!MvrGetEndButtHeight(m_pMotor,m_MotionNum,&buttPos.z))
         buttPos.z=0;
      buttPos.z+=height;
      m_pMotor->SetPosition(&buttPos);
      m_pMotor->StartMotion(m_MotionNum);
   }
}

void cCombatManeuver::NotifyAboutFrameFlags(const int flag)
{
   Assert_(m_pMotor);

   ObjID obj=m_pMotor->GetObjID();

   if((flag&MF_TRIGGER2) && !m_IsPhysical) // then make physical
   {
      if(!MakeAIWeaponPhysical(obj,m_WeaponObj))
      {
         ConfigSpew("MnvrTrace",("%d: could not physicalize swing\n",obj));

         // stop the motion
         m_pMotor->StopMotion(m_MotionNum);
         return;
      }
      ConfigSpew("MnvrTrace",("%d: physicalized swing\n",obj));
      if(m_ActionType==kMSAT_GeneralBlock)
         WeaponHaloInflate(obj,m_WeaponObj);
      m_IsPhysical=TRUE;

   } 
   if((flag&MF_TRIGGER3) && m_IsPhysical)  // then make non-physical
   {
      MakeAIWeaponNonPhysical(obj,m_WeaponObj);
      if(m_ActionType==kMSAT_GeneralBlock)
         WeaponHaloDeflate(obj,m_WeaponObj); // @NOTE: this may get called twice
      m_IsPhysical=FALSE;
      ConfigSpew("MnvrTrace",("%d: de-physicalizing on flag\n",m_pMotor->GetObjID()));
   }
   MvrProcessStandardFlags(m_pMotor,m_pCoord,flag);
}
