/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactrng.cpp,v 1.18 2000/02/11 18:27:21 bfarquha Exp $
//
// AI Action - ranged attack
//

// #define PROFILE_ON 1

#include <comtools.h>
#include <appagg.h>

#include <iobjsys.h>
#include <objdef.h>
#include <traitman.h>

#include <mvrflags.h>
#include <objpos.h>
#include <creatext.h> // for CreatureExists blah
#include <weapcb.h>

#include <aiactrng.h>
#include <aiprabil.h>
#include <aidebug.h>

#include <phprop.h>
#include <rendprop.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAttackRangedAction
//

cAIAttackRangedAction::~cAIAttackRangedAction()
{
   delete m_pMotionAction;
   delete m_pLaunchAction;
}

////////////////////////////////////////

void cAIAttackRangedAction::SetupMotions()
{
   // @HACK: we assume that all creatures want to use a motion
   if (CreatureExists(m_pAIState->GetID()))
   {
      m_pMotionAction = CreateMotionAction(ownerData);
      m_pMotionAction->AddTags("RangedCombat");

      m_MotionStarted = FALSE;
      m_MotionCompleted = FALSE;

      m_LaunchReady = FALSE;
   }
   else
   {
      m_MotionStarted = TRUE;
      m_MotionCompleted = TRUE;

      m_LaunchReady = TRUE;
   }

   m_Launched = FALSE;

   if (!m_pMotionAction)
      m_MotionCompleted = TRUE;
}


////////////////////////////////////////

void cAIAttackRangedAction::Set(ObjID target, ObjID projectile, BOOL lead_target, int accuracy,
                                int fire_count, int launch_joint)
{
   SetupMotions();

   // Launch action
   m_pLaunchAction = new cAILaunchAction(pOwner, ownerData);
   m_pLaunchAction->Set(target, projectile, lead_target, accuracy, fire_count, launch_joint);

   AutoAppIPtr_(TraitManager, pTraitMan);
   m_ProjectileArch = pTraitMan->GetArchetype(projectile);

   m_targetObj = target;
}

////////////////////////////////////////

void cAIAttackRangedAction::Set(const cMxsVector &target, ObjID projectile, int accuracy,
                                int fire_count, int launch_joint)
{
   SetupMotions();

   // Launch action
   m_pLaunchAction = new cAILaunchAction(pOwner, ownerData);
   m_pLaunchAction->Set(target, projectile, accuracy, fire_count, launch_joint);
}

////////////////////////////////////////

void cAIAttackRangedAction::Set(ObjID target, int targetSubModel, ObjID projectile, BOOL lead_target, int accuracy,
                                int fire_count, int launch_joint)
{
   SetupMotions();

   // Launch action
   m_pLaunchAction = new cAILaunchAction(pOwner, ownerData);
   m_pLaunchAction->Set(target, targetSubModel, projectile, lead_target, accuracy, fire_count, launch_joint);

   AutoAppIPtr_(TraitManager, pTraitMan);
   m_ProjectileArch = pTraitMan->GetArchetype(projectile);

   m_targetObj = target;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIAttackRangedAction::Update()
{
   if (InProgress())
   {
      if (m_LaunchFailed)
      {
         result = kAIR_Fail;
         return result;
      }
      if (m_pMotionAction)
      {
         // Check if the motion has hit a launch flag
         if (PollMotionFlags(m_pAIState->GetID(), MF_FIRE_RELEASE))
         {
            m_LaunchReady = TRUE;

            // Create the projectile from the archetype, if it's not the first one
            if (m_Launched)
            {
               AutoAppIPtr_(ObjectSystem, pObjSys);
               ObjID new_projectile = pObjSys->Create(m_ProjectileArch, kObjectConcrete);
               // Remove its physics and make it invisible
               g_pPhysTypeProp->Delete(new_projectile);
               ObjSetHasRefs(new_projectile, FALSE);

               m_pLaunchAction->SetProjectile(new_projectile);
            }

            result = kAIR_NoResultSwitch;
            return result;
         }
      }

      // Finished the motion
      if (m_MotionCompleted)
      {
         // If we haven't launched at least once, launch now
         if (!m_Launched)
         {
            m_LaunchReady = TRUE;
            result = kAIR_NoResultSwitch;
            return result;
         }

         // If there isn't a launch pending, we're done
         if (!m_LaunchReady)
         {
            result = kAIR_Success;
            return result;
         }
      }
      else
      {
         Assert_(m_pMotionAction);
         eAIResult motion_result = m_pMotionAction->Update();

         if (motion_result == kAIR_Success)
         {
            m_MotionCompleted = TRUE;
            result = kAIR_NoResultSwitch;
         }
         else
            result = kAIR_NoResult;

         return result;
      }
   }

   return kAIR_NoResult;
}

////////////////////////////////////////

DECLARE_TIMER(cAIAttackRangedAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIAttackRangedAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIAttackRangedAction_Enact);

   // Ready to fire?
   if (m_LaunchReady)
   {
      m_LaunchReady = FALSE;
      m_Launched = TRUE;

      WeaponEvent(kStartRangedAttack, m_pAIState->GetID(), OBJ_NULL);
      if (m_pLaunchAction->Enact(deltaTime) == kAIR_Fail)
         m_LaunchFailed = TRUE;
      WeaponEvent(kEndRangedAttack, m_pAIState->GetID(), OBJ_NULL);
      result = kAIR_NoResultSwitch;

      return kAIR_NoResult;
   }

   // Motion startup
   if (!m_MotionStarted)
   {
      Assert_(m_pMotionAction);
      if (m_targetObj != OBJ_NULL)
      {
         m_pMotionAction->SetFacing(m_pAIState->AngleTo(ObjPosGet(m_targetObj)->loc.vec));
         m_pMotionAction->SetFocus(m_targetObj, m_rotationSpeed);
      }
      m_pMotionAction->Enact(deltaTime);
      m_MotionStarted = TRUE;
      WeaponEvent(kStartRangedWindup, m_pAIState->GetID(), OBJ_NULL);
   }
   else
      m_pMotionAction->Enact(deltaTime);

   return kAIR_NoResult;
}










