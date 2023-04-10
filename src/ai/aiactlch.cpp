/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactlch.cpp,v 1.32 2000/02/11 18:27:12 bfarquha Exp $
//
// AI Action - launch a projectile
//

// #define PROFILE_ON 1

#include <math.h>
#include <comtools.h>
#include <appagg.h>

#include <objpos.h>
#include <wrtype.h>
#include <port.h>
#include <prjctile.h>
#include <creatext.h>
#include <rendprop.h>
#include <iobjsys.h>
#include <objdef.h>
#include <tweqctrl.h>
#include <crjoint.h>
#include <pgrpprop.h>
#include <traitman.h>
#include <cfgdbg.h>
#include <dmgmodel.h>

#include <phprop.h>
#include <phoprop.h>
#include <physapi.h>
#include <phconst.h>

#include <aiactlch.h>
#include <aicbrmpr.h>   // should move AIGetLaunchOffset somewhere else

#include <scrptapi.h>

#include <ambbase.h>
#include <ambprop.h>

// Must be last header
#include <dbmem.h>

extern IAmbientSoundProperty* g_pAmbientProp;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILaunchAction
//

cAILaunchAction::~cAILaunchAction()
{
   if (m_projectile != OBJ_NULL)
   {
      AutoAppIPtr(ObjectSystem);
      pObjectSystem->Destroy(m_projectile);
   }
}

void cAILaunchAction::Set(ObjID target, ObjID projectile, BOOL lead_target, int accuracy,
                          int launch_count, int launch_joint)
{
   Assert_(ObjPosGet(target));

   m_targetObj = target;
   m_targetSubModel = -1;

   m_projectile = projectile;
   m_leadTarget = lead_target;
   m_accuracy = accuracy;
   m_launchCount = launch_count;
   m_launchJoint = launch_joint;
}

////////////////////////////////////////

void cAILaunchAction::Set(const cMxsVector &target, ObjID projectile, int accuracy,
                          int launch_count, int launch_joint)
{
   m_targetLoc = target;
   m_targetObj = OBJ_NULL;

   m_projectile = projectile;
   m_leadTarget = FALSE;
   m_accuracy = accuracy;
   m_launchCount = launch_count;
   m_launchJoint = launch_joint;
}

////////////////////////////////////////

void cAILaunchAction::Set(ObjID target, int targetSubModel, ObjID projectile, BOOL lead_target, int accuracy,
                          int launch_count, int launch_joint)
{
   Assert_(ObjPosGet(target));

   m_targetObj = target;
   m_targetSubModel = targetSubModel;

   m_projectile = projectile;
   m_leadTarget = lead_target;
   m_accuracy = accuracy;
   m_launchCount = launch_count;
   m_launchJoint = launch_joint;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAILaunchAction::Update()
{
   // @TBD: Instantaneous...should I ever get called?
   return kAIR_Success;
}

////////////////////////////////////////

DECLARE_TIMER(cAILaunchAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAILaunchAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAILaunchAction_Enact);

   if (!CanEnact())
      return result;

   cPhysAttrProp *pAttrProp;
   mxs_vector    *pInitVel;

   mxs_vector actual_target;
   mxs_vector delta;
   mxs_vector dir;
   mxs_vector start_loc;
   mxs_vector launch_point;

   int flags = 0;

   // Compute start loc
   Location start, end, hit;

   if (m_launchJoint > 0)
   {
      launch_point = GetCreatureJointPos(m_pAIState->GetID(), m_launchJoint);
      MakeLocationFromVector(&end, &launch_point);
   }
   else
   {
      if (!AIGetLaunchOffset(m_pAIState->GetID(), &end))
      {
         launch_point = ObjPosGet(m_pAIState->GetID())->loc.vec;
         MakeLocationFromVector(&end, &launch_point);
      }
   }
   MakeHintedLocationFromVector(&start, &ObjPosGet(m_pAIState->GetID())->loc.vec, &ObjPosGet(m_pAIState->GetID())->loc);

   if (!PortalRaycast(&start, &end, &hit, 0))
   {
      // we actually need to back up a tiny bit because the hit location
      // is sometimes actually considered "out of world" due to epsilons
      mx_interpolate_vec(&start_loc, &start.vec, &hit.vec, 0.95);
   }
   else
      start_loc = end.vec;

   // Compute target location, in case it's changed (this is kinda dumb)
   if (m_targetObj != OBJ_NULL)
   {
      if (m_targetSubModel>=0)
         PhysGetSubModLocation(m_targetObj, m_targetSubModel, &m_targetLoc);
      else
      {
         Position *pTargPos;
         if ((pTargPos = ObjPosGet(m_targetObj)) != NULL)
            m_targetLoc = pTargPos->loc.vec;
         else
            return kAIR_Success;
      }
   }

   mx_sub_vec(&delta, &m_targetLoc, &start_loc);

   // Add physics and make visible
   g_pPhysTypeProp->Create(m_projectile);

   // Start up any ambient sounds on the projectile
   AmbientSound *snd;
   if (g_pAmbientProp->Get(m_projectile,&snd))
   {
      snd->flags=snd->flags & ~AMBFLG_S_TURNEDOFF;
      g_pAmbientProp->Set(m_projectile,snd);
   }

   // Ref it
   ObjSetHasRefs(m_projectile, TRUE);

   if (!g_pPhysAttrProp->Get(m_projectile, &pAttrProp))
   {
      Warning(("cAIAttackRangedAction::Enact(): no physics attributes on projectile %d, for AI %d\n",
                m_projectile, m_pAIState->GetID()));
      AutoAppIPtr(ObjectSystem);
      pObjectSystem->Destroy(m_projectile);
      m_projectile = OBJ_NULL;
      return result;
   }

   if (!g_pPhysInitVelProp->Get(m_projectile, &pInitVel))
   {
      Warning(("cAIAttackRangedAction::Enact(): no initial velocity on projectile %d, for AI %d\n",
                m_projectile, m_pAIState->GetID()));
      AutoAppIPtr(ObjectSystem);
      pObjectSystem->Destroy(m_projectile);
      m_projectile = OBJ_NULL;
      return result;
   }

   // Target leading
   if (DoLeadTarget())
   {
      Assert_(m_targetObj != OBJ_NULL);

      mxs_vector target_velocity;
      mxs_vector target_lead;
      mxs_real   time_to_impact;

      PhysGetVelocity(m_targetObj, &target_velocity);
      target_velocity.z *= 0.1;

      time_to_impact = mx_mag_vec(&delta) / pInitVel->x;
      mx_scale_vec(&target_lead, &target_velocity, time_to_impact);

      mx_add_vec(&actual_target, &m_targetLoc, &target_lead);
   }
   else
      actual_target = m_targetLoc;

   mx_sub_vec(&dir, &actual_target, &start_loc);

   if (m_flags&kAILA_HeadingTolerance)
   {
      mxs_vector* pMyVec = &ObjPosGet(m_pAIState->GetID())->loc.vec;
      floatang targetHeading = floatang(pMyVec->x, pMyVec->y, actual_target.x, actual_target.y);
      floatang headingDelta = Delta(m_pAIState->GetFacingAng(), targetHeading);
      if (headingDelta>m_headingEpsilon)
      {
         // get rid of the projectile
         AutoAppIPtr(ObjectSystem);
         pObjectSystem->Destroy(m_projectile);
         m_projectile = OBJ_NULL;
         return kAIR_Fail;
      }
   }

   CalcArc(&dir, pInitVel->x, pAttrProp->gravity);

   mxs_vector add_dir;
   for (int i=0; i<m_launchCount; i++)
   {
      int accuracy = 4 - m_accuracy;

      // Factor in accuracy
      if (accuracy > 0)
      {
         add_dir.x = 0.5 * ((float)(AIRandom(0, accuracy * 25) - (accuracy * 12.5))) / 100.0;
         add_dir.y = 0.5 * ((float)(AIRandom(0, accuracy * 25) - (accuracy * 12.5))) / 100.0;
         add_dir.z = 0.25 * ((float)(AIRandom(0, accuracy * 25) - (accuracy * 12.5))) / 100.0;

         mx_scaleeq_vec(&add_dir, mx_mag_vec(&dir));
         mx_addeq_vec(&dir, &add_dir);
      }

      ObjID firing_projectile;
      if (i == 0)
      {
         firing_projectile = m_projectile;
      }
      else
      {
         // We need to clone our firing one, if we're shooting multiples
         AutoAppIPtr_(ObjectSystem, pObjSys);
         firing_projectile = pObjSys->Create(m_projectile, kObjectConcrete);
      }

      Fire(m_pAIState->GetID(), start_loc, dir, firing_projectile);

      BroadcastAction(start_loc, dir);
   }

   m_projectile = OBJ_NULL;
   return kAIR_Success;
}

////////////////////////////////////////

void cAILaunchAction::Fire(ObjID AIObj, mxs_vector start_loc, mxs_vector dir, ObjID projectile)
{
   launchProjectile(AIObj, projectile, 1.0, 0, NULL, &dir, &start_loc);

   TweqProcessAll(projectile, kTweqDoActivate);

   AutoAppIPtr(ScriptMan);
   sScrMsg msg(projectile, "Launch");
   pScriptMan->SendMessage(&msg);

   if (ObjIsParticleGroup(projectile))
       ObjParticleSetActive(projectile, TRUE);

   // check for failure to launch
   if (!PhysObjHasPhysics(projectile))
   {
      AutoAppIPtr_(DamageModel, pDamMod);

      pDamMod->SlayObject(projectile, AIObj);
   }
}

///////////////////////////////////////////////////////////////////////////////
