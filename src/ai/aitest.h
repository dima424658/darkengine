/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitest.h,v 1.4 1998/05/22 01:24:05 TOML Exp $
//
//
//

#ifndef __AITEST_H
#define __AITEST_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITest
//

class cAITest : public cAIAbility
{
public:

   STDMETHOD_(const char *, GetName)();

   // Scheduling calls
   STDMETHOD_(void, ConnectAbility)(unsigned ** ppSignals);

   // Set/Get whether this is the ability currently running the AI
   STDMETHOD_(void, SetControl)(BOOL);
   STDMETHOD_(BOOL, InControl)();

   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestMode)   (sAIModeSuggestion *);
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   cAITimer m_timer;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AITEST_H */
