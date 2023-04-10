/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactrng.h,v 1.11 1999/04/17 13:14:23 JON Exp $
//
// AI Action - ranged attack
//

#ifndef __AIACTRNG_H
#define __AIACTRNG_H

#include <aiapiiai.h>
#include <aibasact.h>
#include <aiactmot.h>
#include <aiactlch.h>


#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAttackRangedAction
//
// Attack a target with a ranged attack
//

typedef int eAIAttackRangedFlags;
enum eAIAttackRangedFlags_ {kAIRA_RotationSpeed = 0x0001,};

class cAIAttackRangedAction : public cAIAction
{
public:
   cAIAttackRangedAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIAttackRangedAction();

   // Set up action parameters
   void Set(ObjID target, ObjID projectile, BOOL lead_target, int accuracy, 
            int fire_count, int launch_joint);
   void Set(const cMxsVector &target, ObjID projectile, int accuracy, 
            int fire_count, int launch_joint);
   void Set(ObjID target, int targetSubModel, ObjID projectile, BOOL lead_target, int accuracy, 
            int fire_count, int launch_joint);
   
   // Set rotation speed & launch tolerance, must call *after* main set
   void SetRotationParams(fix rotationSpeed, floatang headingEpsilon);

   // Accessors into the launch action
   ObjID              GetTargetObj() const;
   const cMxsVector * GetTargetLoc() const;
   ObjID              GetProjectile() const;
   BOOL               DoLeadTarget() const;

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:

   void SetupMotions();

   BOOL m_MotionStarted;
   BOOL m_MotionCompleted;

   BOOL m_LaunchReady;
   BOOL m_Launched;
   BOOL m_LaunchFailed;

   ObjID m_ProjectileArch;

   cAIMotionAction *m_pMotionAction;
   cAILaunchAction *m_pLaunchAction;

   ObjID m_targetObj;

   int m_flags;
   fix m_rotationSpeed;
};

////////////////////////////////////////

inline cAIAttackRangedAction::cAIAttackRangedAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_AttackRanged, pOwner, data),
   m_pMotionAction(NULL),
   m_pLaunchAction(NULL),
   m_LaunchFailed(FALSE),
   m_targetObj(OBJ_NULL),
   m_flags(0)
{
}

////////////////////////////////////////

inline void cAIAttackRangedAction::SetRotationParams(fix rotationSpeed, floatang headingEpsilon)
{
   m_flags |= kAIRA_RotationSpeed;
   m_rotationSpeed = rotationSpeed;
   Assert_(m_pLaunchAction);
   m_pLaunchAction->SetHeadingTolerance(headingEpsilon);
}

////////////////////////////////////////

inline ObjID cAIAttackRangedAction::GetTargetObj() const
{
   AssertMsg(m_pLaunchAction, "Attempt to get target object with null launch action");
   return m_pLaunchAction->GetTargetObj();
}

////////////////////////////////////////

inline const cMxsVector * cAIAttackRangedAction::GetTargetLoc() const
{
   AssertMsg(m_pLaunchAction, "Attempt to get target location with null launch action");
   return m_pLaunchAction->GetTargetLoc();
}

////////////////////////////////////////

inline BOOL cAIAttackRangedAction::DoLeadTarget() const
{
   AssertMsg(m_pLaunchAction, "Attempt to get target leading status with null launch action");
   return m_pLaunchAction->DoLeadTarget();
}

#pragma pack()

#endif /* !__AIACTRNG_H */









