/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaitrt.h,v 1.4 1998/07/24 13:36:33 JON Exp $
//
//
//

#ifndef __SHKAITRT_H
#define __SHKAITRT_H

#include <aicombat.h>
#include <shkaijra.h>
#include <shkaigna.h>
#include <shkaitpr.h>

#pragma once
#pragma pack(4)

typedef int eAITurretState;
enum eAITurretState_ {kAITurretInactive, kAITurretActive, kAITurretActivating, kAITurretDeactivating};

class cAICombatTurret : public cAICombat
{
public:
   cAICombatTurret();
   ~cAICombatTurret();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   // Goals
   STDMETHOD (SuggestGoal)(cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
   STDMETHOD_(void, OnAlertness) (ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw);

private:
   ObjID GetGun(void);
   BOOL IsDoing(const cAIActions & previous, tAIActionType type);
   cAIJointRotateAction *m_pRotateAction;
   cAIGunAction *m_pGunAction;
   eAITurretState m_state;
   int m_rotateSchemaHandle;

   HRESULT ActiveSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
   HRESULT DeactivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
   HRESULT ActivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
};



#pragma pack()

#endif /* !__AICBTRNG_H */
