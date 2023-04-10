/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactlch.h,v 1.10 1999/06/10 16:24:55 JON Exp $
//
// AI Action - launch a projectile
//

#ifndef __AIACTLCH_H
#define __AIACTLCH_H

#include <aiapiiai.h>
#include <aibasact.h>


#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILaunchAction
//
// Launch a projectile from an AI
//

typedef int eAILaunchActionFlags;

enum eAILaunchActionFlags_ {kAILA_HeadingTolerance = 0x0001,};

class cAILaunchAction : public cAIAction
{
public:
   cAILaunchAction(IAIActor * pOwner, DWORD data = 0);
   cAILaunchAction(IAI *pAI, void *netmsg);
   ~cAILaunchAction();
   
   void Set(ObjID target, ObjID projectile, BOOL lead_target, int accuracy, 
            int launch_count, int launch_joint);
   void Set(const cMxsVector &target, ObjID projectile, int accuracy, 
            int launch_count, int launch_joint);
   void Set(ObjID target, int targetSubModel, ObjID projectile, BOOL lead_target, int accuracy, 
            int launch_count, int launch_joint);

   void SetProjectile(ObjID projectile);
   void SetHeadingTolerance(floatang epsilon);
   
   // Accessors to paramters
   ObjID              GetTargetObj() const;
   const cMxsVector * GetTargetLoc() const;
   ObjID              GetProjectile() const;
   BOOL               DoLeadTarget() const;
   int                GetAccuracy() const;
   int                GetLaunchCount() const;

   // Enact firing in a network proxy (no action exists).
   static void EnactFire(IAI *pAI, void *pMsg);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   // Called by Enact & EnactFire.  Doesn't require that a launch action exist.
   static void Fire(ObjID AIObj, mxs_vector start_loc, mxs_vector dir, ObjID projectile);

   void BroadcastAction(mxs_vector start_loc, mxs_vector dir);

   eAILaunchActionFlags m_flags;

   ObjID      m_targetObj;
   int        m_targetSubModel;
   cMxsVector m_targetLoc;

   ObjID      m_projectile;

   BOOL       m_leadTarget;
   int        m_accuracy;
   int        m_launchCount;
   int        m_launchJoint;

   floatang   m_headingEpsilon;
};

////////////////////////////////////////

inline cAILaunchAction::cAILaunchAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Launch, pOwner, data),
   m_targetObj(OBJ_NULL),
   m_targetLoc(0, 0, 0),
   m_projectile(OBJ_NULL),
   m_leadTarget(FALSE),
   m_flags(0)
{
}

////////////////////////////////////////

inline void cAILaunchAction::SetProjectile(ObjID projectile)
{
   m_projectile = projectile;
}

////////////////////////////////////////

inline void cAILaunchAction::SetHeadingTolerance(floatang epsilon)
{
   m_flags |= kAILA_HeadingTolerance;
   m_headingEpsilon = epsilon;
}

////////////////////////////////////////

inline ObjID cAILaunchAction::GetTargetObj() const
{
   return m_targetObj;
}

////////////////////////////////////////

inline const cMxsVector * cAILaunchAction::GetTargetLoc() const
{
   return &m_targetLoc;
}

////////////////////////////////////////

inline ObjID cAILaunchAction::GetProjectile() const
{
   return m_projectile;
}

////////////////////////////////////////

inline BOOL cAILaunchAction::DoLeadTarget() const
{
   return m_leadTarget;
}

////////////////////////////////////////

inline int cAILaunchAction::GetAccuracy() const
{
   return m_accuracy;
}

////////////////////////////////////////

inline int cAILaunchAction::GetLaunchCount() const
{
   return m_launchCount;
}

#pragma pack()

#endif /* !__AIACTLCH */


