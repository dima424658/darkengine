/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidbgcmp.h,v 1.5 1998/06/08 17:06:20 TOML Exp $
//
//
//

#ifndef __AIDBGCMP_H
#define __AIDBGCMP_H

#include <aibascmp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFlowDebugger
//

typedef cAIComponentBase<IAIComponent, &IID_IAIComponent> cAIFlowDebuggerBase;

class cAIFlowDebugger : public cAIFlowDebuggerBase
{
public:   
   cAIFlowDebugger();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction *);
   STDMETHOD_(void, OnGoalProgress)(const cAIGoal *);
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionChange)(IAIAction * pPrevious, IAIAction * pAction);

private:
};

///////////////////////////////////////

inline cAIFlowDebugger::cAIFlowDebugger()
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDBGCMP_H */
