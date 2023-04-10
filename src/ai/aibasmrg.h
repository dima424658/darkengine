/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasmrg.h,v 1.8 1998/10/30 12:27:17 TOML Exp $
//
//
//

#ifndef __AIBASMRG_H
#define __AIBASMRG_H

#include <aiapimov.h>
#include <aibascmp.h>
#include <aimovsug.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveRegulator
//
// Base class of all movement regulators
//

class cAIMoveRegulator : public cAIComponentBase<IAIMoveRegulator, &IID_IAIMoveRegulator>
{
public:
   cAIMoveRegulator(eAITimerPeriod period = AICustomTime(200));
   virtual ~cAIMoveRegulator();

   STDMETHOD_(BOOL, NewRegulations)();
   
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &);

   STDMETHOD_(BOOL, AssistGoal)(const sAIMoveGoal & goal,
                                sAIMoveGoal * pResultGoal);

   STDMETHOD_(BOOL, WantsInterrupt)();

protected:
   cAIMoveSuggestions m_Suggestions;
   cAITimer           m_Timer;
};

///////////////////////////////////////

inline cAIMoveRegulator::cAIMoveRegulator(eAITimerPeriod period)
 : m_Timer(period)
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBASMRG_H */
