/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimplyr.h,v 1.2 1999/01/14 18:05:10 KATE Exp $
//
//
//

#ifndef __AIMPLYR_H
#define __AIMPLYR_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITestMotionPlayer
//

class cAITestMotionPlayer : public cAIAbility
{
public:

   STDMETHOD_(const char *, GetName)();

   // Scheduling calls
   STDMETHOD_(void, ConnectAbility)(unsigned ** ppSignals);

   // Set/Get whether this is the ability currently running the AI
   STDMETHOD_(void, SetControl)(BOOL);
   STDMETHOD_(BOOL, InControl)();

   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestMode)   (sAIModeSuggestion *);
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIMPLYR_H */
