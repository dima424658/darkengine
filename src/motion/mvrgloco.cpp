/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrgloco.cpp,v 1.39 1999/08/05 17:09:46 Justin Exp $
// This is the maneuver builder/controller for solid ground locomotions -
// That is, one's in which the creature is affected by gravity and moving along
// the floor beneath its feet.
//
// XXX Need to add ability to check stride length and guesstimate end pos
//
// XXX end state stuff for maneuvers needs to be calculated.  I think 
// this should only have 2D position, since this faster to calculate and is
// all I think AI will really want anyway (yes?)
//
// XXX deal with case when no motion capture is found
//
// XXX apply gravity when a foot should be in contact with sground but isn't.
// This should help with recovering from miscalculations of end ground height

#include <mmanuver.h>
#include <mcoord.h>
#include <wrtype.h>
#include <portal.h>
#include <mvrutils.h>
#include <fastflts.h>
#include <mschbase.h>
#include <physcast.h>
#include <ghostmvr.h>

#include <fix.h>
#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>
#include <mschutil.h>
#include <rand.h>
#include <textarch.h>

#include <math.h> // until fastflts works
      
// must be last header
#include <dbmem.h>

class cGroundLocoManeuverFactory: public cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord);      

   virtual IManeuver *LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile);
};

typedef struct
{
   BOOL m_DoRotate;
   BOOL m_UseBend;
   BOOL m_IsBeingDeleted;
   mxs_angvec m_NewDirection;
   mxs_angvec m_ExpectedEndFacing;
   mxs_vector m_ExpectedEndPos;
   mxs_vector m_StrideXlat;
   float m_ButtZOffset;
   int m_MotionNum;
   mps_motion_param m_MotionParam;
   mxs_ang m_RotAng;
   mxs_ang m_MotRotAng;
   sMGaitSkillData *m_pGaitData;
   int m_SchemaID;
   BOOL m_FirstStride;
} sGroundLocoData;

class cGroundLocoManeuver: public cManeuver, private sGroundLocoData
{
public:
   cGroundLocoManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, const cMotionSchema *pSchema, const sMcMoveParams& params);
   cGroundLocoManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success);
   ~cGroundLocoManeuver();

////////
// Motion System Client Functions
////////
   virtual void GetExpectedEndMoveState(sMcMoveState& moveState);

////////
// Motion Coordinator Functions
////////
   virtual void Execute();

   virtual void Save(ITagFile *pTagFile);

////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect() {}
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel) \
      { MvrCalcSlidingCollisionResponse(pForces,nForces,pCurrentVel,pNewVel,kMvrUCF_MaintainSpeed); }
   virtual void NotifyAboutBeingStuck();

//
// Motion status notification functions
//
   virtual void NotifyAboutFrameFlags(const int flags) { MvrProcessStandardFlags(m_pMotor,m_pCoord, flags); }
   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags);

private:
};

// XXX really want to return to stand position, but can't since end callback
// doesn't deal with more than one mo-cap in maneuver
inline void cGroundLocoManeuver::NotifyAboutBeingStuck()
{
   // XXX HACK: only update the phys models if doing non-combat locomotion,
   // since combat ones have the whole problem of being asked to go in
   // wacky directions where butt precedes knees.  KJ 2/98
   m_pMotor->SetOrientation(&m_ExpectedEndFacing,m_UseBend);
}

cGroundLocoManeuverFactory g_GroundLocoManeuverFactory;
EXTERN cManeuverFactory *g_pGroundLocoManeuverFactory=&g_GroundLocoManeuverFactory;


cMotionPlan *cGroundLocoManeuverFactory::CreatePlan(const cMotionSchema *pSchema,const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   cGroundLocoManeuver *pMnvr;
   sMcMoveParams usedParams=params;

   if(!(params.mask&kMotParmFlag_Direction))
   {
      // being told to locomote, but not in any direction.  Try to figure out
      // what request means.
      ConfigSpew("MnvrTrace",("Told to locomote, but not given a direction"));
      if(params.mask&kMotParmFlag_Facing)
      {
         usedParams.direction=usedParams.facing;
         usedParams.mask|=kMotParmFlag_Direction;
      } else
      {
         // assume direction wants to be current direction         
         Assert_(pMotor);
         const sMotorState *pState=pMotor->GetMotorState();
         usedParams.direction=pState->facing;
         usedParams.mask|=kMotParmFlag_Direction;
      }
   }

   pMnvr = new cGroundLocoManeuver(pMotor, pCoord, pSchema, usedParams);

   AssertMsg(pMnvr,"could not alloc maneuver");

   cMotionPlan *pPlan= new cMotionPlan;

   AssertMsg(pPlan,"could not alloc plan");

   pPlan->Prepend(pMnvr);

   return pPlan;
}

IManeuver *cGroundLocoManeuverFactory::LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile)
{
   BOOL success;
   cGroundLocoManeuver *pMnvr=new cGroundLocoManeuver(pMotor,pCoord,pTagFile,&success);

   if(!success)
   {
      delete pMnvr;
      return NULL;
   }
   return pMnvr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// GroundLoco MANEUVER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define MAX_INTERESTED_MOTIONS 10

#include <locobase.h> // for footfall flags

cGroundLocoManeuver::cGroundLocoManeuver(IMotor *pMotor, cMotionCoordinator *pCoord, ITagFile *pTagFile,BOOL *success)
{
   *success=TRUE;
   ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
   ITagFile_Move(pTagFile,(char *)((sGroundLocoData *)this),sizeof(*((sGroundLocoData*)this)));
   m_pMotor=pMotor;
   m_pCoord=pCoord;
   m_pGaitData=NULL;
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
      Warning(("cGroundLocoManuever: cannot load maneuver because creature is out of synch\n"));
      m_MotionNum=-1;
      *success=FALSE;
   }
}

void cGroundLocoManeuver::Save(ITagFile *pTagFile)
{
   // write app data (cManeuver stuff)
   ITagFile_Move(pTagFile,(char *)&m_AppData,sizeof(m_AppData));
   ITagFile_Move(pTagFile,(char *)((sGroundLocoData *)this),sizeof(*((sGroundLocoData *)this)) );
}


cGroundLocoManeuver::cGroundLocoManeuver(IMotor *pMotor, cMotionCoordinator *pCoord,\
    const cMotionSchema *pSchema, const sMcMoveParams& params )
{
   BOOL turning=FALSE;
   int i;
   float dist;

   m_pMotor=pMotor;
   m_pCoord=pCoord;   

   if(pSchema)
      m_SchemaID=pSchema->GetSchemaID();
   else
      m_SchemaID=kMotSchemaID_Invalid;

   m_IsBeingDeleted=FALSE;

   sMcMotorState *pS=m_pCoord->GetInterMnvrState();

   // check if just did this schema
   if(pS->lastSchemaID!=m_SchemaID)
   {
      ConfigSpew("MnvrTrace",("starting to %d\n",m_SchemaID));
      m_FirstStride=TRUE;
   } else
   {
      ConfigSpew("MnvrTrace",("still %d-ing\n",m_SchemaID));
      m_FirstStride=FALSE;
   }

// Get skill and motor information
//
   const sMotorState *pState=m_pMotor->GetMotorState();

   Assert_(pSchema);

   if(!MSchUGetGaitData(pSchema,&m_pGaitData))
   {
      m_pGaitData=NULL;
      Assert_(FALSE);
   }

   float timeWarp=m_pGaitData->timeWarp;
   float tw;
   if(pSchema->GetTimeWarp(&tw))
      timeWarp*=tw;
   if(m_pGaitData->noise)
   {
      float noise=(2*m_pGaitData->noise*Rand())/RAND_MAX;
      timeWarp=(1.0-m_pGaitData->noise+noise)*timeWarp; // since 0<=noise<=2*m_pGaitData->noise;
   }

   int off=-1;

// Decide whether or not to use turn
//
// XXX using mxs_ang instead of degrees for wrapping
// XXX this doesn't distinguish between starting with left or right foot in turn
// XXX get facing vs direction to work correctly

   mxs_ang turnTol=degrees_to_fixang(m_pGaitData->turnTolerance);
   mxs_ang adelta=pState->facing.tz-params.direction.tz;
   mxs_ang minDelta=MX_ANG_PI,diff;
   sMGaitTurn *pTurn=m_pGaitData->turns;

   // check if delta outside turn tolerance
   if(m_pGaitData->numTurns>0 &&  ((adelta<=MX_ANG_PI&&adelta>turnTol)||(adelta>=MX_ANG_PI&&adelta<2*MX_ANG_PI-turnTol)))
   {
      turning=TRUE;
      for(i=0;i<m_pGaitData->numTurns;i++,pTurn++)
      {
         mxs_ang angle=degrees_to_fixang(pTurn->angle);
         diff=angle-adelta;
         if(diff>MX_ANG_PI)
            diff=2*MX_ANG_PI-diff;
         if(diff<=minDelta)
         {
            minDelta=diff;
            off=pTurn->mot;
         }
      }
   }

// Decide whether to do left or right stride, if not turning
//
   if(!turning)
   {
      if(pState->contactFlags&LEFT_FOOT) // XXX this will only work for bipeds
      {
         off=m_pGaitData->rightStride;
      } else
      {
         off=m_pGaitData->leftStride;
      }
   }

   m_MotionNum=-1;
#ifdef NETWORK_LOCOS   
   if (IsRemoteGhost(pMotor->GetObjID()))
   {
      m_MotionNum = GetGhostMotionNumber();
#ifndef SHIP
      if (config_is_defined("ghost_motion_spew"))
      {
         mprintf("Remote ghost %d playing schema %d, motion %d\n", pMotor->GetObjID(), m_SchemaID, m_MotionNum);
      }
#endif
   }
#endif   
   if (m_MotionNum==-1) // not a remoteghost, or remghost didnt want to deal 
   {
      if(!pSchema->GetMotion(off,&m_MotionNum))
         m_MotionNum=-1;
   }

   if(m_MotionNum==-1)
   {
      Warning(("GroundLocoManeuver: unable to find motion at offset %d\n",off));
   }

   if (IsLocalGhost(pMotor->GetObjID()))
   {
#ifndef SHIP
      if (config_is_defined("ghost_motion_spew"))
      {
         mprintf("rGloco: Local ghost %d informing remote about schema %d, motion %d\n", 
                 pMotor->GetObjID(), m_SchemaID, m_MotionNum);
      }
#endif
      // this is actually going to be intercepted inside here, and we're
      // going to do something tricky, which is why the reception of gloco
      // maneuvers is still in the NETWORK_LOCOS up there...
      GhostSendMoCap(pMotor->GetObjID(),m_SchemaID,m_MotionNum,TRUE);
   }

   mxs_vector xlat;
   float duration;
   sMotionPhys phys;

   m_MotionParam.flags=NULL;

//   if(!MvrGetRawMovement(m_pMotor,m_MotionNum, xlat, duration,m_ButtZOffset))
   if(!MvrGetRawMovement(m_pMotor,m_MotionNum, phys))
   {
   // couldn't get motion capture. just rotate
      m_DoRotate=TRUE;
      m_NewDirection=params.direction;
      mx_zero_vec(&m_StrideXlat);
      m_RotAng=m_NewDirection.tz-pState->facing.tz;
      m_UseBend=FALSE;
      m_MotRotAng=0;
      return;
   }
   xlat=phys.xlat;
   m_MotRotAng=phys.endDir;
   duration=phys.duration;
   m_ButtZOffset=phys.buttZOffset;

   m_DoRotate=TRUE;

   float stretch=1.0;

   if(pSchema->GetDistance(&dist))
   {
      if(phys.distance>0.1)
         stretch=dist/phys.distance;
   } else 
   {
      pSchema->GetStretch(&stretch);
   }
   if(m_pGaitData->stretch!=1.0) // get multiplied by schema stretch
   {
      stretch*=m_pGaitData->stretch;
   }
   if(stretch!=1.0)
   {
      m_MotionParam.flags|=MP_STRETCH;
      m_MotionParam.stretch=stretch;
      mx_scaleeq_vec(&xlat,stretch);
   } else
   {
      m_MotionParam.stretch=1.0;
   }

   // shrink motion xlat if desired 2D distance to travel is less
   // than xlat
   //

   m_UseBend=!m_FirstStride;  // never bend on first stride

   // calc how much to shrink mocap, and scale xlat accordingly
   if(params.mask&kMotParmFlag_Distance)
   {
      mxs_vector tmp;
      float mag;

      tmp.x=xlat.x;
      tmp.y=xlat.y;
      tmp.z=0;

      ConfigSpew("MnvrTrace",("dist param %g\n",params.distance));

      // check xlat big enough for this to matter
      mag=mx_mag2_vec(&tmp); 
      if(mag>0.3)
      {
         float frac=(params.distance*params.distance)/mag;

         if(frac<1.0) // shrink the motion
         {
            // take sqrt to get accurate frac
            frac=sqrt(frac);

            m_UseBend=FALSE; // since want to hit point exactly
            ConfigSpew("MnvrTrace",("shrink %d by %g\n",m_MotionNum,frac));
            mx_scaleeq_vec(&xlat,frac);
            m_MotionParam.flags|=MP_STRETCH;
            m_MotionParam.stretch*=frac;
         }
      }
   }

   m_NewDirection=params.direction;

// @TODO: modify desired direction of motion, due to
// physical constraints etc.

   m_ExpectedEndFacing=m_NewDirection;

// Compute motion parameters and endpoints
//
   mxs_matrix mat;
   mxs_ang rot;

   // if want to use bend,
   // rotate xlat by facing/2, since motion bending will have this effect
   // (which is different from rotating multiped and then doing motion.
   // NOTE: this assumes that raw translation is along x-axis, so motion
   // must be processed as a locomotion
   
   m_RotAng=m_NewDirection.tz-pState->facing.tz-m_MotRotAng;

   if(m_UseBend)
   {
      if(m_RotAng>MX_ANG_PI)
      {
         rot=2*MX_ANG_PI-((2*MX_ANG_PI-m_RotAng)/2);
      } else
      {
         rot=m_RotAng/2;
      }
   } else
   {
      rot=m_RotAng;
   }
   mx_mk_rot_z_mat(&mat,pState->facing.tz+rot);
   mx_mat_muleq_vec(&mat,&xlat);

   // assume always timewarp, since noise always applied.
   duration*=timeWarp;
   m_MotionParam.flags|=MP_DURATION_SCALE;
   m_MotionParam.duration_scalar=timeWarp;

   // compute motion stretch and bend params
   // XXX NOTE: this won't work properly if distance flag is also
   // set!
   if(params.mask&kMotParmFlag_ExactSpeed)
   {
      // stretch motion by fraction of distance it will cover by default vs
      // distance that we want covered.
      float desiredDist=params.exactSpeed*duration;
      float actualDist=phys.distance;
      float frac=(actualDist>0.1)?(desiredDist/actualDist):1.0;

      mx_scale_vec(&m_StrideXlat,&xlat,frac);
      m_MotionParam.flags|=MP_STRETCH;
      m_MotionParam.stretch=frac;
   } else
   {
      m_StrideXlat=xlat;
   }
}

cGroundLocoManeuver::~cGroundLocoManeuver()
{
   m_IsBeingDeleted=TRUE;

   // Stop motion if one is playing
   if(m_MotionNum>=0 && m_pMotor!= NULL)
      m_pMotor->StopMotion(m_MotionNum);
}

void cGroundLocoManeuver::GetExpectedEndMoveState(sMcMoveState &moveState)
{
   AssertMsg(FALSE,"GetExpectedEndMoveState not implemented for GroundLoco maneuver");
}

#define kVerticalTolerance 0.2 // feet
#define kGroundRaycastOffset 1.5 // feet

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

void cGroundLocoManeuver::Execute()
{
   Assert_(m_pCoord);   
   sMcMotorState *pS=m_pCoord->GetInterMnvrState();
   m_pCoord->ClearInterMnvrState();
   pS->lastSchemaID=m_SchemaID;

   if(m_DoRotate)
   {
      if(m_UseBend)
      {
         ConfigSpew("MnvrTrace",("bend %d\n",fixang_to_degrees(m_RotAng)));
         m_MotionParam.flags|=MP_BEND;
         if(m_RotAng>MX_ANG_PI)
         {
            m_MotionParam.bend=(mx_ang2rad(m_RotAng)-2*MX_REAL_PI)/2;
         } else
         {
            m_MotionParam.bend=mx_ang2rad(m_RotAng/2);
         }
      } else
      {
                                                   // with not-forward-facing motions
         // NOTE: this assumes that direction of action in motion capture
         // is along the x-axis

         ConfigSpew("MnvrTrace",("rotate so dir %x\n",m_NewDirection.tz));

         mxs_angvec dir=m_NewDirection;
         dir.tz-=m_MotRotAng;

         m_pMotor->SetOrientation(&dir,FALSE);
      }
   }
   if(m_MotionNum<0)
   {
      sMcMoveState state;
      // XXX need to rework this end state stuff

      AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
   } else
   {
      // check ground height
      Location curLoc=*m_pMotor->GetLocation();
      mxs_vector newPos;
      Location newLoc,hitLoc,fromLoc;
      ObjID hitObj;
      float offset;
      mxs_real groundHeight;
      BOOL foundGround;
      float desiredZ=0;

      // first, move to ground if not already grounded 
      // @TODO: enter into freefall instead.
      if(MvrFindGroundHeight(m_pMotor->GetObjID(),&curLoc,&groundHeight,&hitObj))
      {
         mxs_vector buttPos;

         MvrGetEndButtHeight(m_pMotor,m_MotionNum,&desiredZ);
         desiredZ+=groundHeight;
         if(fabs(desiredZ-curLoc.vec.z)>kVerticalTolerance)
         {
            mx_copy_vec(&buttPos,&curLoc.vec);
            buttPos.z=desiredZ;
            m_pMotor->SetPosition(&buttPos);
            curLoc.vec.z=desiredZ;
         } 
      } 

      // raycast to see if terrain in the way
      // want to raycast from around knee height, so balconies work etc.
      // NOTE: really don't want to be doing this at all anyway. (KJ 1/98)

      // first find current ground height
      fromLoc=curLoc;
      if(MvrFindGroundHeight(m_pMotor->GetObjID(),&fromLoc,&groundHeight,&hitObj))
      {
         fromLoc.vec.z=groundHeight+kGroundRaycastOffset;
      }
      mx_add_vec(&newPos,(mxs_vector *)&fromLoc.vec,&m_StrideXlat);
      MakeHintedLocationFromVector(&newLoc,&newPos,&curLoc);

      if (PhysRaycast(fromLoc, newLoc, &hitLoc, &hitObj, 0.0) == kCollideNone)
      {
         // find ground height at end of stride
         foundGround=MvrFindGroundHeight(m_pMotor->GetObjID(), &newLoc,&groundHeight,&hitObj);
      } else
      {
         // find ground height at just before terrain collision
         // XXX this should not really be fractional, but 'tis simpler
         mx_interpolate_vec(&newLoc.vec,&curLoc.vec,&hitLoc.vec,0.95);
         foundGround=MvrFindGroundHeight(m_pMotor->GetObjID(),&newLoc,&groundHeight,&hitObj);
      }

      // NOTE: this should work even though newPos.z isn't new butt
      // height, since newpos term cancels out
      mx_copy_vec(&m_ExpectedEndPos,&newPos);
      if(foundGround)
      {
         offset=(groundHeight+m_ButtZOffset-(curLoc.vec.z+m_StrideXlat.z));
         m_ExpectedEndPos.z=groundHeight+m_ButtZOffset;
      } else
      { 
         hitObj = OBJ_NULL;
         offset=0;
         m_ExpectedEndPos.z=curLoc.vec.z+m_StrideXlat.z;
      }

      // set the standing obj
      MvrSetStandingObj(m_pMotor->GetObjID(), hitObj);

      // and set its offset
      if (!IsTextureObj(hitObj))
         MvrSetStandingOffset(m_pMotor->GetObjID(), hitObj, &m_ExpectedEndPos);
      else
      {
         mxs_vector offset = {0, 0, m_ButtZOffset};
         MvrSetTerrStandingOffset(m_pMotor->GetObjID(), &offset);
      }

//      if(ffabsf(offset)>kVerticalTolerance)
      if(fabs(offset)>kVerticalTolerance)
      {
         
         // XXX checking vertical tolereances
         // should really happen at motion coordinator level, I think,
         // since AI can force non-physical velocities also.  Should be up
         // to coordinator or cerebellum to decide to discard path because of
         // ground height or passing through terrain.  Doing it here for
         // now, since there is currently no path checking

         AssertMsg(m_pGaitData,"no gait data");
         // truncate offset to max for gait
         offset=min(offset,m_pGaitData->maxAscend);
         offset=max(offset,m_pGaitData->maxDescend);

         m_MotionParam.flags|=MP_VINC;
         m_MotionParam.vinc=offset;
      }
      if(m_FirstStride)
         m_pMotor->StartMotionWithParam(m_MotionNum,&m_MotionParam,kMotStartFlag_ForceBlend);
      else
         m_pMotor->StartMotionWithParam(m_MotionNum,&m_MotionParam);
   }
}

// XXX need to rework this end state stuff
void cGroundLocoManeuver::NotifyAboutMotionEnd(int motionNum, int frame, ulong flags)
{
   sMcMoveState state;

   AssertMsg(m_pCoord,"No motion coordinator for maneuver!");
   // only tell coordinator if maneuver not in middle of being deleted
   if(!m_IsBeingDeleted&&motionNum==m_MotionNum)
   {
      mxs_vector diff,pos;
      mxs_angvec ang;

      // see if really ended up where thought it would
      m_pMotor->GetTransform(&pos,&ang);
      mx_sub_vec(&diff,&m_ExpectedEndPos,&pos);
      diff.z=0;
      if(config_is_defined("MnvrTrace"))
      {
         ConfigSpew("MnvrTrace",("delta %g, %g  mag %g   ang %d\n",diff.x,diff.y,mx_mag_vec(&diff),\
            fixang_to_degrees(ang.tz-m_NewDirection.tz) ));
      }
      m_pCoord->NotifyAboutManeuverCompletion(this,state);
   }
}
