/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigna.h,v 1.8 1999/11/19 14:52:19 adurant Exp $
//
// AI Action - gun attack
//

This file has been moved to aigunact.h  AMSD


#ifndef __SHKAIGNA_H
#define __SHKAIGNA_H

#include <aiapiiai.h>
#include <aibasact.h>
#include <aicblink.h>
#include <aiactlch.h>
#include <relation.h>
#include <simtime.h>
#include <shkgnbas.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGunAction
//
// Attack a target with a gun
//

typedef int eAIGunActionState;
enum eAIGunActionState_ {kAIGunActionStart, kAIGunActionBurst, kAIGunActionEnd,};

class cAIGunAction : public cAIAction
{
public:
   cAIGunAction(IAIAbility * pOwner, DWORD data = 0);

   // Set up action parameters
   void Set(ObjID target, int subModel = 0, int gunJointID = -1);

   // One time gun shoot for a proxy in a network game.
   // Called by cAIShockProxyBehaviorSet
   static void EnactProxyShootGun(IAI *pAI, void *netmsg);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   eAIGunActionState m_state;
   ObjID m_gunID;
   int m_shotsTaken;             // no of shots we have fired
   sAIGunDesc *m_pAIGunDesc;
   sBaseGunDesc *m_pBaseGunDesc;
   tSimTime m_startTime;
   ObjID m_targetObj;
   int m_subModel;
   int m_gunJointID;

   cAIGunAction(IAI *pAI);  // A constructor that is used only internally.
   void Initialize(); // Initialization used by both constructors
   void SetState(eAIGunActionState state);
   void Shoot(void);
   void BroadcastShoot();
   BOOL CheckTarget(void);
};

////////////////////////////////////////

inline BOOL cAIGunAction::CheckTarget(void)
{
   return (g_pAIAttackLinks->GetSingleLink(m_pAIState->GetID(), m_targetObj) != LINKID_NULL);
}

////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIGNA_H */









