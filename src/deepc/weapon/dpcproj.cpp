/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include "dpcproj.h"

extern "C"
{
#include <matrix.h>
}

#include <mprintf.h>

#include <matrixc.h>
#include <appagg.h>
#include <traitman.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objpos.h>
#include <dmgbase.h>
#include <portal.h>
#include <camera.h>
#include <playrobj.h>
#include <objsys.h>
#include <propman.h>
#include <prjctile.h>
#include <psnd.h>
#include <schema.h>
#include <rand.h>

#include <stimprop.h>   // source scaling

#include <phprop.h>
#include <phoprop.h>
#include <phcore.h>
#include <physapi.h>

#include <dpccheck.h>
#include <gunprop.h>
#include <dpcpjbas.h>
#include <dpcprjpr.h>

#include <property.h>
#include <propbase.h>

#include <sdesc.h>
#include <sdestype.h>
#include <sdesbase.h>

#include <netmsg.h>
#include <netprops.h>

#include <aiman.h>

// must be last header
#include <dbmem.h>

// from prjctile.cpp
extern BOOL has_projectile_type(ObjID obj);

sLaunchParams g_defaultLaunchParams = 
{
   0,
   1.0,
   1.0,  
   {0, 0, 0}, 
   0,    
   0, 
   0,
   0,
   NULL, 
};

//////////
//
// Networking projectiles
//
// The general design for projectiles is that they are local-only objects;
// this is more or less necessary due to timing issues. (Otherwise, the
// projectile might get destroyed on its host machine before it has a
// chance to hit anything on the client.)
//
// So, we treat them at a high level -- we broadcast the message to create
// the projectile, and let each machine deal with the effects individually.
// In particular, each client is responsible for deciding whether its local
// player gets hit. This *can* lead to mild anomalies (eg, two players
// getting hit by the same bullet, or it getting destroyed without hitting
// anything that can take damage), but they look to be uncommon and mild
// enough that we can live with them.
//
// @TBD: does this work right if the projectile already exists?
//
ObjID DPCFireProjectile(ObjID owner, ObjID archetype, int flags,
                          float intensityMult, mxs_vector *pPos,
                          mxs_angvec *pAng, mxs_vector *pVel);

static cNetMsg *g_pFireProjectileMsg = NULL;

static void handleFireProjectile(ObjID owner, ObjID archetype, int flags,
                                 float intensityMult, mxs_vector *pPos,
                                 mxs_angvec *pAng, mxs_vector *pVel)
{
   ObjID bullet = DPCFireProjectile(owner, archetype, flags, intensityMult,
                                      pPos, pAng, pVel);
   if (bullet != OBJ_NULL) {
      // Note that this is just a copy of the "real" bullet; this will
      // affect damage later:
      gLocalCopyProp->Set(bullet, TRUE);
   }
}

static sNetMsgDesc sFireProjectileDesc =
{
   kNMF_Broadcast,
   "FireProj",
   "Fire Projectile",
   NULL,
   handleFireProjectile,
   {{kNMPT_SenderObjID, kNMPF_None, "Owner"},
    {kNMPT_AbstractObjID, kNMPF_None, "Archetype"},
    {kNMPT_Int, kNMPF_None, "Flags"},
    {kNMPT_Float, kNMPF_None, "Intensity Multiplier"},
    {kNMPT_Vector, kNMPF_None, "Position"},
    {kNMPT_Block, kNMPF_None, "Angle", sizeof(mxs_angvec)},
    {kNMPT_Vector, kNMPF_None, "Velocity"},
    {kNMPT_End}}
};

//
// Tell everyone that we're firing a projectile.
// This runs on the shooter's machine.
//
// This only runs if we're firing a local-only projectile, which is the
// usual, but not universal, case. If we're firing a hosted projectile,
// then we let it be created normally, and the send a message giving its
// velocity.
//
static void BroadcastFireProjectile(ObjID owner, ObjID archetype, int flags,
                                    float intensityMult, mxs_vector *pPos,
                                    mxs_angvec *pAng, mxs_vector *pVel)
{
   if (NetworkCategory(archetype) == kLocalOnly)
   {
      g_pFireProjectileMsg->Send(OBJ_NULL, owner, archetype, flags, 
                                 intensityMult, pPos, pAng, pVel);
   }
}

//
// The other side of the equation -- hosted projectiles.
//
// If a projectile is long-lived, then it should be hosted. In that case,
// we only create it on its creating host, and tell the other machines
// about its trajectory.
//
static cNetMsg *g_pProjTrajMsg = NULL;

static void handleProjTraj(ObjID bulletID,
                           mxs_vector *pPos,
                           mxs_angvec *pAng,
                           mxs_vector *pVel)
{
   PhysSetModLocation(bulletID, pPos);
   PhysSetModRotation(bulletID, pAng);
   PhysSetVelocity(bulletID, pVel);
}

static sNetMsgDesc sProjTrajDesc =
{
   kNMF_Broadcast,
   "ProjTraj",
   "Projectile Trajectory",
   NULL,
   handleProjTraj,
   {{kNMPT_SenderObjID, kNMPF_None, "Bullet"},
    {kNMPT_Vector, kNMPF_None, "Position"},
    {kNMPT_Block, kNMPF_None, "Angle", sizeof(mxs_angvec)},
    {kNMPT_Vector, kNMPF_None, "Velocity"},
    {kNMPT_End}}
};

static void BroadcastProjectileTrajectory(ObjID bulletID,
                                          mxs_vector *pPos,
                                          mxs_angvec *pAng,
                                          mxs_vector *pVel)
{
   if (NetworkCategory(bulletID) == kHosted)
   {
      g_pProjTrajMsg->Send(OBJ_NULL, bulletID, pPos, pAng, pVel);
   }
}

//////////

ObjID CannisterSpawn(ObjID projID, sCannister *pCannister)
{
   ObjID  newID;
   mxs_vector vel;
   mxs_vector angvel;
   mxs_angvec ang;
   mxs_vector *pNorm = NULL;
   mxs_matrix rotMat;
   cMxsVector unitVec(1, 0, 0);
   mxs_real dp;
   ObjID archID;
   ITraitManager *pTraitMan = AppGetObj(ITraitManager);
   IObjectSystem *pObjSys = AppGetObj(IObjectSystem);
   sCannister newCannister = *pCannister;
   Position *pos = ObjPosGet(projID);

   // find where physics last hit (what a hack!)
   if ((PortalRaycastCell != -1) && (PortalRaycastFindPolygon() != -1))
      pNorm = (mxs_vector *) &WR_CELL(PortalRaycastCell)->plane_list[PortalRaycastPlane].normal;
   // else we have no clue what to reflect off...
   do
   {
      // angle between 0x0000 and 0xffff
      ang.tx = Rand() << 1;
      ang.ty = Rand() << 1;
      ang.tz = Rand() << 1;
      // make rot matrix
      mx_ang2mat(&rotMat,&ang);
      // rotate unit vector by random amount
      mx_mat_mul_vec(&vel, &rotMat, &unitVec);
      // test dot prod
      if (pNorm == NULL)
         break;
      dp = mx_dot_vec(&vel, pNorm);
   } while (dp<0);
   
   mx_scaleeq_vec(&vel, pCannister->m_speed);

   // angular velocity between -pi/2 and pi/2 (-90 and 90 degrees)
   angvel.x = ((Rand() % 314) - 157) / 100.0;
   angvel.y = ((Rand() % 314) - 157) / 100.0;
   angvel.z = ((Rand() % 314) - 157) / 100.0;

   archID = pTraitMan->GetArchetype(projID);
   SafeRelease(pTraitMan);
   newID = pObjSys->BeginCreate(archID, kObjectConcrete);
//   newID = pObjSys->Create(archID, kObjectConcrete);
   if (newID != OBJ_NULL)
   {
      // set all of our fields that we care about
      ObjPosUpdate(newID, &(pos->loc.vec), &ang);

      cPhysTypeProp typeProp;
      typeProp.type = kSphereProp;
      g_pPhysTypeProp->Set(newID, &typeProp);

      pObjSys->EndCreate(newID);

      pObjSys->CloneObject(newID, projID);
      SafeRelease(pObjSys);

      PhysSetSubModRadius(newID, 0, 0);
      PhysSetModLocation(newID, &(pos->loc.vec));
      PhysSetModRotation(newID, &ang);

      // set the initial velocity and angular velocity
      PhysSetVelocity(newID, &vel);
      PhysSetRotationalVelocity(newID, &angvel);
   
      newCannister.m_generations -=1;
      CANNISTER_SET(newID, &newCannister);
   
   }
   else
      Warning(("Cannister spawn - obj Create says no free objects!\n"));
   return newID;
}

// for when a projectile strikes terrain
eDamageResult DPCProjTerrImpactHandler(ObjID projID)
{
   // cannister producing objects
   sCannister *pCannister;
   if (CannisterGet(projID, &pCannister))
   {
      int i;
      sCannister oldCannister = *pCannister;
      
      if (pCannister->m_generations>0)
         for (i = 0; i < pCannister->m_num; i++)
            CannisterSpawn(projID, pCannister);
      oldCannister.m_generations = 0;
      CANNISTER_SET(projID, &oldCannister);
   }
   return kDamageNoOpinion;
}

// @HACK: following code stolen from prjctile.cpp because we want to be able to
// modify the projectile start position relative to firer.

static ObjID projArch = ROOT_ARCHETYPE;

inline void ApplyRandomization(mxs_ang randAngle, mxs_ang *pPitch, mxs_ang *pHeading)
{
   // add random angle 
   if (randAngle != 0)
   {
      mxs_ang randAdjust = RandRange(0, 2*randAngle)-randAngle;
      (*pHeading) += randAdjust;
      randAdjust = RandRange(0, 2*randAngle)-randAngle;
      (*pPitch) += randAdjust;
   }
}

// Having done the calculations in DPCLaunchProjectile, this actually
// fires the thing. Note that, while DPCLaunchProjectile only gets run
// on the host that actually fires the bullet, this gets run on every
// machine.
static ObjID DPCFireProjectile(ObjID owner, ObjID archetype, int flags,
                                 float intensityMult, mxs_vector *pPos,
                                 mxs_angvec *pAng, mxs_vector *pVel)
{
   ObjID bulletID = OBJ_NULL;
   BOOL new_object = !OBJ_IS_CONCRETE(archetype);

   if (new_object && (archetype == OBJ_NULL)) {
      archetype = projArch;
   }
   
   if (new_object) {
      bulletID = BeginObjectCreate(archetype, kObjectConcrete);
   } else {
      bulletID = archetype;
   }

   if (bulletID == OBJ_NULL) {
      // Should this be an assertion?
      Warning(("DPCMakeProjectile: couldn't create new projectile!\n"));
      return OBJ_NULL;
   }

   // apply stimulus intensity modifier
   if (intensityMult != 1)
   {
      float scale;

      if (g_pSourceScaleProperty->Get(bulletID, &scale))
         intensityMult *= scale;
      g_pSourceScaleProperty->Set(bulletID, intensityMult);
   }

   ObjPosUpdate(bulletID, pPos, pAng);

   // If we're really launching a "projectile" then inform physics
   if (has_projectile_type(bulletID))
      PhysSetProjectile(bulletID);

   // Finish off creation
   if (new_object)
      EndObjectCreate(bulletID);
      
   // Inform the AI
   if ((flags&kLaunchTellAI) && ((flags&kLaunchCheckPlayerHead) || IsPlayerObj(owner)))
   {
      AutoAppIPtr(AIManager);
      if (!!pAIManager)
         pAIManager->NotifyFastObj(PlayerObject(), bulletID, *pVel);
   }

   // Set up physics
   PhysSetModLocation(bulletID, pPos);
   PhysSetModRotation(bulletID, pAng);

   // Set velocity
   PhysSetVelocity(bulletID, pVel);

   BroadcastProjectileTrajectory(bulletID, pPos, pAng, pVel);

   SetProjectileFirer(bulletID, owner);

   return (bulletID);
}

// this is a hacked down, simplified version of launchProjectileVel
// All of the calculations for generating the projectile should go here.
ObjID DPCLaunchProjectile(ObjID owner, ObjID archetype, sLaunchParams *pParams)
{
   ObjPos* ownerPos = ObjPosGet(owner);
   mxs_vector *pInitVel;
   mxs_vector defvel, baseVel = {0, 0, 0};
   mxs_vector basePos = ownerPos->loc.vec;
   mxs_vector pos;
   mxs_angvec baseAng = ownerPos->fac;
   mxs_angvec ang;
   mxs_matrix mat;
   sProjectile *pProjectile;
   int spray;
   mxs_ang spread;
   ObjID projID;
   mxs_vector vel;

   if (pParams == NULL)
      pParams = &g_defaultLaunchParams;

   // Launch heading & pitch
   if ((pParams->flags)&kLaunchBankOverride)
      baseAng.tx = pParams->bank;
   else
      baseAng.tx += pParams->bank;
   if ((pParams->flags)&kLaunchPitchOverride)
      baseAng.ty = pParams->pitch;
   else
      baseAng.ty += pParams->pitch;
   if ((pParams->flags)&kLaunchHeadingOverride)
      baseAng.tz = pParams->heading;
   else
      baseAng.tz += pParams->heading;
   mx_ang2mat(&mat, &baseAng);

   // Launch position
   if ((pParams->flags)&kLaunchLocOverride)
      pos = pParams->loc;
   else
   {
      pos = basePos;
      for (int j=0; j<3; j++)
         mx_scale_addeq_vec(&pos, &mat.vec[j], pParams->loc.el[j]);
   }

   // Check start pos
   if ((pParams->flags)&(kLaunchCheck|kLaunchCheckPlayerHead))
   {
      Location hitLoc;
      BOOL good = true;
      Location validLoc;
      Location *pValidLoc;
      Location testLoc;

      if ((pParams->flags)&kLaunchCheckPlayerHead)
      {
         // setup valid location from player head 
         mxs_vector headVec;

         PhysGetSubModLocation(PlayerObject(), PLAYER_HEAD, &headVec);
         MakeHintedLocationFromVector(&validLoc, &headVec, &(ObjPosGet(PlayerObject())->loc));
         pValidLoc = &validLoc;
      }
      else
      {
         // pParams->flags&kLaunchCheck
         if (pParams->pValidLoc != NULL)
            pValidLoc = pParams->pValidLoc;
         else
            pValidLoc = &(ownerPos->loc);
      }
      testLoc.vec = pos;
      UpdateChangedLocation(&testLoc);
      good = PortalRaycast(pValidLoc, &testLoc, &hitLoc, 0);
      if (good)
      {
         cPhysTypeProp* pPhysType;

         // @TODO: this isn't working - figure out why
         // Is this the right way to test if archetype is a sphere?
         if (g_pPhysTypeProp->Get(archetype, &pPhysType) && (pPhysType->type == kSphereProp))
         {
            cPhysDimsProp* pPhysDims;
            float radius;

            if (g_pPhysDimsProp->Get(archetype, &pPhysDims))
               // assume no sub-models...
               radius = pPhysDims->radius[0];
            else
               radius = 0;
            good = DPCCheckSphereLocation(testLoc, radius);
         }
      }
      if (!good)
         pos = pValidLoc->vec;
   }

   // Apply global randomization to heading & pitch
   ApplyRandomization(pParams->error, &baseAng.ty, &baseAng.tz);

   // Get the default physics property for the archetype
   if (!OBJ_IS_CONCRETE(archetype) && 
       (g_pPhysInitVelProp->Get(archetype, &pInitVel)))
      defvel = *pInitVel;
   else
      mx_mk_vec(&defvel, 1, 0, 0);

   if ((pParams->flags)&kLaunchRelativeVelocity)
      PhysGetVelocity(owner, &baseVel);

   // Now, iterate over each projectile in spray (if any)
   if (ProjectileGet(archetype, &pProjectile))
   {
      spray = pProjectile->m_spray;
      spread = pProjectile->m_spread;
   }
   else
   {
      spray = 1;
      spread = 0;
   }

   for (int i=0; i<spray; i++)
   {
      // Apply individual randomization
      ang = baseAng;
      ApplyRandomization(spread, &ang.ty, &ang.tz);
      
      // Create final rotation matrix
      mx_ang2mat(&mat, &ang);
      
      // Add in default velocity
      vel = baseVel;
      for (int j=0; j<3; j++)
         mx_scale_addeq_vec(&vel, &mat.vec[j], defvel.el[j]);
      
      // Apply speed multiplier
      if (pParams->speedMult != 1)
      {
         mxs_matrix rotMat;
         cMxsVector xVec(1, 0, 0);
         mxs_vector normVel;
         float norm;

         norm = mx_norm_vec(&normVel, &vel);
         mx_mk_move_x_mat(&rotMat, &normVel);
         mx_mat_mul_vec(&vel, &rotMat, &xVec);
         mx_scaleeq_vec(&vel, norm*pParams->speedMult);
      }
      
      // Now, tell everyone to actually fire the thing:
      BroadcastFireProjectile(owner, archetype, pParams->flags, pParams->intensityMult, &pos, &ang, &vel);
      projID = DPCFireProjectile(owner, archetype, pParams->flags, pParams->intensityMult, &pos, &ang, &vel);
   }
   return projID;
}

//////////
//
// Startup and Shutdown
//

void DPCProjectileInit(void)
{
   g_pFireProjectileMsg = new cNetMsg(&sFireProjectileDesc);
   g_pProjTrajMsg = new cNetMsg(&sProjTrajDesc);
}

void DPCProjectileTerm(void)
{
   delete g_pFireProjectileMsg;
   delete g_pProjTrajMsg;
}
