/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aincbdmg.h,v 1.6 1999/05/10 19:10:20 JON Exp $
//
//
//

#ifndef __AINCBDMG_H
#define __AINCBDMG_H

#include <ainoncbt.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitNonCombatDamageResponseAbility(IAIManager *);
BOOL AITermNonCombatDamageResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINonCombatDamageResponse
//

class cAINonCombatDamageResponse : public cAINonCombatAbility
{
public:
   cAINonCombatDamageResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID);
   
   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
private:
   BOOL     m_fDamageSignal;
   BOOL     m_severe;
   cAITimer m_SpeakAmbushTimer;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#endif /* !__AINCBDMG_H */
