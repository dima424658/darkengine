/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkairca.h,v 1.1 1999/03/25 17:39:20 JON Exp $
//
//
//

#ifndef __SHKAIRCA_H
#define __SHKAIRCA_H

#include <aimultcb.h>
#include <aiprrngd.h>

#include <shkaircp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL ShockAIInitRangedCombatAbility(IAIManager *);
BOOL ShockAITermRangedCombatAbility();

///////////////////////////////////////////////////////////////////////////////

typedef int eAIShockRangedRange;
typedef int eAIShockRangedState;

class cAIShockRangedSubcombat  : public cAISubcombat
{
public:
   cAIShockRangedSubcombat();
   ~cAIShockRangedSubcombat();

   STDMETHOD_(void, Init)();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnDamage)(const sDamageMsg *pMsg);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   void Reset();

   void SetState(eAIShockRangedState state);

   cAIAction* SuggestCloseAction(void);
   cAIAction* SuggestRetreatAction(void);
   cAIAction* SuggestFireAction(void);

   void EvaluatePosition(void);
   void EvaluateRange(void);
   BOOL SelectProjectile(void);
   BOOL ProjectileHittable(int targ_method, ObjID projectile, ObjID source, ObjID target);
   BOOL HasTargetLOS(void);

   // cache the prop
   sShockRanged* m_pShockRangedProp;

   eAIShockRangedState m_state;

   // evaluated state
   float m_dist;
   eAIShockRangedRange m_range;
   // chosen proj
   ObjID m_selectedProjID;
   sAIProjectileRel* m_pSelectedProjData;
};

#pragma pack()

#endif /* !__SHKAIRCA_H */


