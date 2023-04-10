/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiidle.h,v 1.9 1998/10/21 22:20:39 TOML Exp $
//
//
//

#ifndef __AIIDLE_H
#define __AIIDLE_H

#include <ainoncbt.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitIdleAbility(IAIManager *);
BOOL AITermIdleAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIIdle
//

class cAIIdle : public cAINonCombatAbility
{
public:
   cAIIdle();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Reset timers if idle goal wins
   STDMETHOD_(void, OnSimStart)();
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnDefend)(const sAIDefendPoint *);
   
   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   cAIRandomTimer m_GestureTimer;
   cAIRandomTimer m_DirTimer;
   cAITimer       m_DefendTimer;
   cAITimer       m_TurnTimeout;
   cAITimer       m_OriginTimer;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIIDLE_H */
