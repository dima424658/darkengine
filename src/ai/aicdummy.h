/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicdummy.h,v 1.4 1998/07/15 18:12:17 TOML Exp $
//

#pragma once
#ifndef __AICDUMMY_H
#define __AICDUMMY_H

#include <ainoncbt.h>
#include <aiutils.h>

#pragma pack(4)

// create the properties needed for CombatDummies
BOOL AIInitCombatDummyAbility(IAIManager *);
BOOL AITermCombatDummyAbility(void);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombatDummy
//

class cAICombatDummy : public cAINonCombatAbility
{
public:
   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   
   // Update the status of the current goal
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnGameEvent)(void *magic);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // did we mention we hate everyone
   STDMETHOD_(void, OnBeginFrame)(void);

private:
   // Reset timers if idle goal wins
   STDMETHOD_(BOOL, DummyTagsAvailable)(void);

   int m_BlockCount;         // if > 0, count of "blocking idles" in a row
   int m_BlockDirection;
   int m_IHateEveryone;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICDUMMY_H */
