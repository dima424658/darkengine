/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasabl.h,v 1.14 1999/11/29 12:27:45 BFarquha Exp $
//
// Base implementation of an AI ability
//

#ifndef __AIBASABL_H
#define __AIBASABL_H

#include <aiapiabl.h>
#include <aibasatr.h>
#include <aictlsig.h>

#pragma once
#pragma pack(4)

class cAIPath;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAbility
//

class cAIAbility : public cAIActorBase<IAIAbility, &IID_IAIAbility>
{
public:
   cAIAbility();
   ~cAIAbility();

   // Hook up
   STDMETHOD_(void, ConnectAbility)(unsigned ** ppSignals);

   // Default initializtion
   STDMETHOD_(void, Init)();

   // Save/load
   STDMETHOD_(BOOL, SaveGoal)(ITagFile * pTagFile, cAIGoal * pGoal);
   STDMETHOD_(BOOL, LoadGoal)(ITagFile * pTagFile, cAIGoal ** ppGoal);

   // Set/Get whether this is the ability currently running the AI
   STDMETHOD_(void, SetControl)(BOOL);
   STDMETHOD_(BOOL, InControl)();

   // Base ability will automatically signal goal on sim start
   STDMETHOD_(void, OnSimStart)();

   // Base ability will automatically signal goal on mode change greater than efficient
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);

   // Base ability will automatically signal action if winner of goal decision
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);

   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestMode)   (sAIModeSuggestion *);
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD_(BOOL, FinalizeGoal)(cAIGoal *pGoal);

protected:

   //
   // Notification helpers
   //

   // Did the goal come from this ability?
   BOOL IsOwn(const cAIGoal * pGoal) const;

   // Is the goal message even relevant to this ability?
   BOOL IsRelevant(const cAIGoal * pPrevious, const cAIGoal * pGoal) const;

   // Is the ability newly gaining control?
   BOOL IsGainingControl(const cAIGoal * pPrevious, const cAIGoal * pGoal) const;

   // Is the ability losing control?
   BOOL IsLosingControl(const cAIGoal * pPrevious, const cAIGoal * pGoal) const;

   // Did the action come from this ability? (have here as goal overload hides base)
   BOOL IsOwn(IAIAction * pAction) const;

   // Is the action message even relevant to this ability? (have here as goal overload hides base)
   BOOL IsRelevant(IAIAction * pPrevious, IAIAction * pAction) const;

   //
   // Save/Load
   //
   void BaseAbilitySave(ITagFile *);
   void BaseAbilityLoad(ITagFile *);

   //
   // Signalling protocol
   //
   void Signal(unsigned signals);
   void ClearSignal(unsigned signals);

   void SignalMode();
   void SignalGoal();
   void SignalAction();

   //
   // Flags, extendible by derived
   //
   enum
   {
      // Other constants
      kInControl = 0x01,

      kBaseFirstAvailFlag = 0x02
   };

   unsigned  m_flags;

private:
   unsigned m_signals;

#ifndef SHIP
   //
   // Debugger variables, as MSVC has trouble looking up into the component template
   //

   // Used only for setting breakpoints
   ObjID             m_DebugID;
   IInternalAI *     m_pDebugAI;
   const cAIState *  m_pDebugAIState;
#endif
};

///////////////////////////////////////

inline cAIAbility::cAIAbility()
 : m_flags(0),
   m_signals(0)
{

}

///////////////////////////////////////

inline void cAIAbility::Signal(unsigned signals)
{
   (m_signals) |= signals;
}

///////////////////////////////////////

inline void cAIAbility::ClearSignal(unsigned signals)
{
   (m_signals) &= ~signals;
}

///////////////////////////////////////

inline void cAIAbility::SignalMode()
{
   (m_signals) |= kAI_SigMode;
}

///////////////////////////////////////

inline void cAIAbility::SignalGoal()
{
   (m_signals) |= kAI_SigGoal;
}

///////////////////////////////////////

inline void cAIAbility::SignalAction()
{
   (m_signals) |= kAI_SigAct;
}

///////////////////////////////////////

inline BOOL cAIAbility::IsOwn(IAIAction * pAction) const
{
   return cAIActorBase<IAIAbility, &IID_IAIAbility>::IsOwn(pAction);
}

///////////////////////////////////////

inline BOOL cAIAbility::IsRelevant(IAIAction * pPrevious, IAIAction * pAction) const
{
   return cAIActorBase<IAIAbility, &IID_IAIAbility>::IsRelevant(pPrevious, pAction);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBASABL_H */
