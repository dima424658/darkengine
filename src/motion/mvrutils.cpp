/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mvrutils.cpp,v 1.37 2000/03/06 09:42:29 adurant Exp $

#include <matrixs.h>
#include <portal.h>
#include <mvrutils.h>
#include <rand.h>
#include <stdlib.h>
#include <lg.h>
#include <motdmnge.h>
#include <wr.h> // for CellFromLoc
#include <motdesc.h>
#include <cfgdbg.h>
#include <motmngr.h>
#include <schema.h>
#include <property.h>
#include <propbase.h>
#include <textarch.h>
#include <prcniter.h>

#include <creature.h>
#include <creatext.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmodobb.h>
#include <phcontct.h>
#include <physcast.h>
#include <phoprop.h>
#include <aipthobb.h>
#include <aiground.h>

#include <mvrflags.h> // for footfall flags
#include <sndgen.h>   // for footfalls

// script stuff
#include <comtools.h>
#include <appagg.h>
#include <scrptapi.h> 
#include <scrptbas.h>
#include <scrptsrv.h>
#include <bodscrpt.h>
#include <puppet.h>

// must be last header
#include <dbmem.h>


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/// PLAN BUILDING UTILITIES
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

inline BOOL MvrIsValidMotion(const int motionNum)
{
   return (motionNum>=0&&motionNum<mp_num_motions);
}

inline void MvrValidateMotion(const int motionNum)
{
   AssertMsg1(MvrIsValidMotion(motionNum),"motion num %d invalid",motionNum);
}

EXTERN mxs_ang MvrGetHeadingAngle(mxs_vector *v)
{
   mxs_ang angle;

   angle=mx_rad2ang(acos(v->x));
   if(angle>0)
   {
      if(v->y<0) // -pi/2 < actual angle < 0
         angle=-angle;
   } else if (v->y>0) // -pi < actual angle <= -pi/2 
      angle=-angle;
   return angle;   
}

////////////////////////////////////////////////////////////////////////

cDynArray<ObjID> standableObjectList;

void InitGroundHeightObjects()
{
   sPropertyObjIter iter;
   ObjID objID;
   int i;

   standableObjectList.SetSize(0);

   // standable objects include pressure plates
   g_pPhysPPlateProp->IterStart(&iter);
   while (g_pPhysPPlateProp->IterNext(&iter, &objID))
   {
      if (OBJ_IS_CONCRETE(objID))
         standableObjectList.Append(objID);
   }
   g_pPhysPPlateProp->IterStop(&iter);

   // and things marked as AI pathable
   cConcreteIter citer(g_pObjPathableProperty);

   citer.Start();
   while (citer.Next(&objID))
   {
      BOOL found = FALSE;

      for (i=0; i<standableObjectList.Size(); i++)
      {
         if (standableObjectList[i] == objID)
         {
            found = TRUE;
            break;
         }
      }
      if (!found)
         standableObjectList.Append(objID);
   }
   citer.Stop();
}

void TermGroundHeightObjects()
{
   standableObjectList.SetSize(0);
}

BOOL MvrFindGroundHeight(ObjID obj, Location *loc, mxs_real *pHeight, ObjID *pGroundObj)
{
   Location toLoc;
   Location hitLoc;
   ObjID    hitObj;
   BOOL     retval = TRUE;

   MakeLocation(&toLoc, loc->vec.x, loc->vec.y, loc->vec.z - 100.0);

   PhysRaycastSetObjlist(standableObjectList, standableObjectList.Size());

   int physcast_val;

   if ((physcast_val = PhysRaycast(*loc, toLoc, &hitLoc, &hitObj, 0.0, kCollideOBB | kCollideTerrain)) == kCollideNone)
   {
      toLoc.vec.z -= 100.0;

      if (PhysRaycast(*loc, toLoc, &hitLoc, &hitObj, 0.0, kCollideOBB | kCollideTerrain) == kCollideNone)
      {
         Warning(("Unable to find ground under %g %g %g\n", loc->vec.x, loc->vec.y, loc->vec.z));

         *pGroundObj = OBJ_NULL;
         *pHeight = 0;
         retval = FALSE;
      }
   }

   if (retval)
   {
      *pGroundObj = hitObj;
      *pHeight=hitLoc.vec.z;
      
      *pHeight = AIAdjustMvrGroundHeight(obj, loc, *pHeight, *pGroundObj);
   }

   PhysRaycastClearObjlist();

   return retval;
}

////////////////////////////////////////////////////////////////////////

void MvrSetStandingObj(ObjID standingObj, ObjID supportObj)
{
   cCreature *pCreature = CreatureFromObj(standingObj);

   // Create contact with our ground object
   if ((standingObj != OBJ_NULL) && (standingObj != supportObj) && !IsTextureObj(supportObj))
   {
      cPhysModel *pModel1 = g_PhysModels.Get(standingObj);
      cPhysModel *pModel2 = g_PhysModels.Get(supportObj);

      // Inform the creature we're standing on it
      pCreature->SetStandingObj(supportObj);

      if (pModel1 != NULL)
      {
         DestroyAllObjectContacts(standingObj, 0, pModel1);
         
         if (pModel2 != NULL)
         {
            int submod = 0;
            int i;

            if (pModel2->GetType(0) == kPMT_OBB)
            {
               mxs_vector normal_list[6];

               // Find the "top" surface of the OBB
               ((cPhysOBBModel *)pModel2)->GetNormals(normal_list);

               mxs_real best_z = -1.0;
               for (i=0; i<6; i++)
               {
                  if (normal_list[i].z > best_z)
                  {
                     submod = i;
                     best_z = normal_list[i].z;
                  }
               }
            }
            CreateObjectContact(standingObj, 0, pModel1, supportObj, submod, pModel2);
            pModel2->SetSleep(FALSE);
         }
      }
   }
   else
      pCreature->SetStandingObj(OBJ_NULL);
}

////////////////////////////////////////

void MvrSetStandingOffset(ObjID standingObj, ObjID supportObj, mxs_vector *pExpectedEndPos)
{
   if (supportObj == OBJ_NULL)
      return;

   cCreature *pCreature = CreatureFromObj(standingObj);

   mxs_vector offset;
   mx_sub_vec(&offset, pExpectedEndPos, &ObjPosGet(supportObj)->loc.vec);

   pCreature->SetStandingOffset(offset);
}

////////////////////////////////////////////////////////////////////////

void MvrSetTerrStandingOffset(ObjID standingObj, mxs_vector *pOffset)
{
   cCreature *pCreature = CreatureFromObj(standingObj);
   pCreature->SetStandingOffset(*pOffset);
}

////////////////////////////////////////////////////////////////////////

#ifdef OLD_WAY
int MvrRandomIntInRange(int low, int high)
{
   int r;

   r = low + (1 + high - low) * Rand() / (RAND_MAX+1);
   if (r > high) r = high;
   return r;
}
#endif
#define MvrRandomIntInRange RandRange

////////////////////////////////////////////////////////////////////////

// XXX TO DO: move this to multiped library
#define mp_motion_get_info(motionNum) &((mp_motion_list+(motionNum))->info)

EXTERN BOOL mp_get_component_id(int *compID, int motionNum, int jointID)
{
	mps_motion * m = mp_motion_list + motionNum;
	mps_comp_motion * cm = m->components;
   int i;

   AssertMsg(m->info.type==MT_CAPTURE,"mp_get_component_id only valid with captured motions");
   for(i=0;i<m->num_components;i++,cm++)
   {
      if(cm->type==CM_ROT&&cm->joint_id==jointID)
      {
         *compID=i;
         return TRUE;
      }
   } 
   return FALSE;
}

// XXX HACK HACK HACK HACK
EXTERN void MotCompRotCallback(mps_motion_info *mi,mps_comp_motion *cm,int frame, quat *data);


EXTERN void mp_get_rot_at_frame(quat *rot, int motionNum, int frame, int compID)
{
	mps_motion * m = mp_motion_list + motionNum;

   AssertMsg(m->info.type==MT_CAPTURE,"mp_get_rot_at_frame only valid with captured motions");
   AssertMsg((m->components+compID)->type==CM_ROT,"mp_get_rot_at_frame: component %d is not a rotation");

//   mp_capture_component_rot_func(&m->info,m->components+compID,frame,rot);
   MotCompRotCallback(&m->info,m->components+compID,frame,rot);
}

// this only works with valid motion-captured motions, not virtual ones.
// this takes into account how the motion will need to translate differently 
// depending on motor size vs that of the motion capture actor.
BOOL MvrGetRawMovement(const IMotor *pMotor, const int motionNum, sMotionPhys &phys)
{
   if(!MvrIsValidMotion(motionNum))
      return FALSE;

   AssertMsg(g_pMotionSet,"No motion set, cannot compute motion movement");

   phys.endDir=g_pMotionSet->GetEndHeading(motionNum);
   g_pMotionSet->GetTranslation(motionNum,&phys.xlat);
   // take into account motion scale -- Kate 1/99
   mx_scaleeq_vec(&phys.xlat,pMotor->GetMotionScale());
   phys.distance=mx_mag_vec(&phys.xlat);
   phys.duration=g_pMotionSet->GetDuration(motionNum);

   return MvrGetEndButtHeight(pMotor, motionNum, &phys.buttZOffset);
}

BOOL MvrGetEndButtHeight(const IMotor *pMotor, int motionNum, float *pHeight)
{
// @HACK: don't want this really.  Want end offset computed by motion processor
// and stored with motion

   *pHeight=pMotor->GetDefaultButtZOffset();
   
   return TRUE;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/// MOTOR RESOLVING UTILITIES
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

#define NUDGE_FAC 0.001
#define MAX_Z_NUDGE 0.1

// assume that first vector on ovel is main phys model.  nvel gets set to 
// what new velocity for this model should be.
void MvrCalcSlidingCollisionResponse(const mxs_vector *pForces, const int nForces, \
const mxs_vector *ovel, mxs_vector *nvel, ulong flags)
{
   float dot;
   int i;
   mxs_vector newdir, off;
   float len,mindot;

#if 0
   mprintf("Force List:\n");
   for (i=0; i<nForces; i++)
      mprintf(" [%d] %g %g %g\n", i, pForces[i].x, pForces[i].y, pForces[i].z);
   mprintf("Orig velocity: %g %g %g\n", ovel->x, ovel->y, ovel->z);
#endif

   // average wall normals affecting submodels 
   // hopefully just linear averaging will be okay
   mx_zero_vec(&newdir);
   for(i=0;i<nForces;i++)
      mx_addeq_vec(&newdir,(mxs_vector *)&pForces[i]);

   if(flags&kMvrUCF_MaintainSpeed)
   {
      if ((fabs(newdir.x) > 0.1) || (fabs(newdir.y) > 0.1))
         newdir.z = 0;
   }

   // this is slow but hopefully infrequent
   // set new facing
   len=mx_mag_vec(&newdir);
   // this is slow but hopefully infrequent
   // set new facing
   if(len>0.1)
      mx_scaleeq_vec(&newdir,1.0/len);
   else
      mx_zero_vec(&newdir);

#if 0
   mprintf("newdir (avg surf normal) = %g %g %g\n", newdir.x, newdir.y, newdir.z);
#endif

   // get rid of component of bip velocity perp to wall if heading towards wall,
   // want to find most negative dot product for velocities of all phys models,
   // since that model was moving furthest in bad direction.
   mindot=99999.0;
   for(i=0;i<nForces;i++)
   {
      dot=mx_dot_vec(&ovel[i],&newdir);
      if(dot<mindot)
         mindot=dot;
   }
   Assert_(mindot!=99999.0);
   ConfigSpew("BipedForceSpew",("terrain norm %g, %g, %g,  dot %g\n",newdir.x,newdir.y,newdir.z,dot));

   if(mindot<0)
   {
      mx_scale_vec(&off,&newdir,-mindot);
   } else
   {
      ConfigSpew("BipedForceSpew",("collided moving AWAY from wall\n"));
      mx_scale_vec(&off,&newdir,mindot);
   }

   // calc translation  
   mx_add_vec(nvel,(mxs_vector *)ovel,&off); // new vel vector

   if(flags&kMvrUCF_MaintainSpeed)
   {
      len=mx_mag_vec(nvel);

      // scale new velocity to match magnitude of old.
      // XXX NOTE: this caused wacky behavior previously when colliding with
      // sloped walls, and is not a physically correct solution.
      mxs_vector maybe_new_vel;

      if(len>0.05)
         mx_scale_vec(&maybe_new_vel,nvel,mx_mag_vec(ovel)/len);

      // Only accept the slide rescale if the z velocity doesn't increase dramatically
      if (fabs(maybe_new_vel.z) < (fabs(nvel->z) * 3))
         mx_copy_vec(nvel, &maybe_new_vel);
   }

#if 0
   mprintf("new velocity = %g %g %g\n", nvel->x, nvel->y, nvel->z);
#endif

   // add in nudge factor to nudge away from wall
   mx_scale_vec(&off,&newdir,NUDGE_FAC);
   // don't nudge up off floor, however, or not by much
   if(off.z>MAX_Z_NUDGE)
      off.z=MAX_Z_NUDGE;
   mx_addeq_vec(nvel,&off);
}

////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/// EXTERNAL SYSTEM INTERACTION UTILITIES
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

EXTERN void FootfallCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   const sMotorState *pState=pMotor->GetMotorState();

   if (flags&MF_LEFT_FOOTFALL)
   {
      mxs_vector pos;
      mxs_angvec rot;

      pMotor->GetTransform(&pos,&rot);

      pMotor->SetContact(pState->contactFlags|LEFT_FOOT);
      FootfallSound(pMotor->GetObjID(), &pos);
   }
   if (flags&MF_RIGHT_FOOTFALL)
   {
      mxs_vector pos;
      mxs_angvec rot;

      pMotor->GetTransform(&pos,&rot);

      pMotor->SetContact(pState->contactFlags|RIGHT_FOOT);
      FootfallSound(pMotor->GetObjID(), &pos);
   }
   if (flags&MF_LEFT_FOOTUP)
      pMotor->SetContact(pState->contactFlags&~LEFT_FOOT);
   if (flags&MF_RIGHT_FOOTUP)
      pMotor->SetContact(pState->contactFlags&~RIGHT_FOOT);
}

EXTERN void MvrInterruptCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   if(!pCoord)
      return;
   if(flags&MF_CAN_INTERRUPT)
      pCoord->NotifyThatCanInterrupt();
}

EXTERN void MvrBodyCollapseCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   Label collapse_schema = {"collapse_player"};

   SchemaPlayLoc(&collapse_schema, &ObjPosGet(objID)->loc.vec);
}

EXTERN void MvrHitCorpseCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
   Label collapse_schema = {"hit_corpse"};

   SchemaPlayLoc(&collapse_schema, &ObjPosGet(objID)->loc.vec);
}

EXTERN void MvrWeaponSwingCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
      mxs_vector pos;
      mxs_angvec rot;

      pMotor->GetTransform(&pos,&rot);
      WeaponSwingSound(pMotor->GetObjID(), &pos);
}

EXTERN void MvrWeaponChargeCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
      mxs_vector pos;
      mxs_angvec rot;

      pMotor->GetTransform(&pos,&rot);
      WeaponChargeSound(pMotor->GetObjID(), &pos);
}

EXTERN void MvrSearchCallback(ObjID objID, IMotor *pMotor, cMotionCoordinator *pCoord, const int flags)
{
      mxs_vector pos;
      mxs_angvec rot;

      pMotor->GetTransform(&pos,&rot);
      SearchSound(pMotor->GetObjID(), &pos);
}

///////////////////////////////////////////////////

EXTERN void MvrProcessStandardFlags(IMotor *pMotor,cMotionCoordinator *pCoord, const int flags)
{
   if(!pMotor)
      return;

   NotifyMotionFlags(pMotor->GetObjID(), flags, pMotor, pCoord);
}

// @TODO: get rid of this once puppet's go away, since start/end provided by
// ai script ability, and flag triggers will be installed as callbacks on obj
// by concerned party (though nobody looks at them yet).
EXTERN void MvrSendMotionMessage(ObjID obj, int motionNum, eMotionStatus whatHappened, ulong flags)
{
   if(!ObjIsPuppet(obj))
      return;

   sBodyMsg::eBodyAction actionType;

   switch(whatHappened)
   {
      case kMotionStart:
         actionType=sBodyMsg::kMotionStart;
         break;
      case kMotionEnd:
         actionType=sBodyMsg::kMotionEnd;
         break;
      case kMotionFlagReached:
         actionType=sBodyMsg::kMotionFlagReached;
         break;
      default:
         AssertMsg1(FALSE,"Unknown motion message type %d for script message\n",whatHappened);
         break;
   }
   AutoAppIPtr(ScriptMan);

   char *name;

   if(NULL!=(name=MotDescGetName(motionNum))) // scriptman can't handle NULL strings KJ 1/98
   {
      sBodyMsg bodyMessage(obj, actionType, name, flags);
      pScriptMan->SendMessage(&bodyMessage);
   }
}

void MvrUtilsRegisterDefaultObjFlags(ObjID obj)
{
   AddMotionFlagListener(obj, MF_FEET_FLAGS, FootfallCallback);
   AddMotionFlagListener(obj, MF_CAN_INTERRUPT, MvrInterruptCallback);

   // This might want to get set upon death throes trigger (E3 hack for now)
   AddMotionFlagListener(obj, MF_TRIGGER4, MvrBodyCollapseCallback);
   AddMotionFlagListener(obj, MF_TRIGGER5, MvrHitCorpseCallback);

   // For creature weapon powerup sounds
   AddMotionFlagListener(obj, MF_TRIGGER6, MvrWeaponChargeCallback);

   // For creature searching panning sounds (Thief 2 robots, for instance)
   AddMotionFlagListener(obj, MF_TRIGGER7, MvrSearchCallback);

   // For creature melee weapon swinging sounds
   AddMotionFlagListener(obj, MF_TRIGGER8, MvrWeaponSwingCallback);

}

void MvrUtilsUnRegisterDefaultObjFlags(ObjID obj)
{
   RemoveMotionFlagListener(obj, MF_FEET_FLAGS);
   RemoveMotionFlagListener(obj, MF_CAN_INTERRUPT);

   // This might want to get unset upon death throes trigger (E3 hack for now)
   RemoveMotionFlagListener(obj, MF_TRIGGER4);
   RemoveMotionFlagListener(obj, MF_TRIGGER5);
   
   RemoveMotionFlagListener(obj, MF_TRIGGER6);      
   RemoveMotionFlagListener(obj, MF_TRIGGER7);
   RemoveMotionFlagListener(obj, MF_TRIGGER8);

}
