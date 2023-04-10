/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactmov.h,v 1.7 1999/03/02 17:42:00 TOML Exp $
//
//
//

#ifndef __AIACTMOV_H
#define __AIACTMOV_H

#include <aiapiiai.h>
#include <aibasact.h>
#include <aimovsug.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIMoveEnactor);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveAction
//
// Used by abilities to make instantaneous move suggestions to the locomotion
// move suggestion system.
//
// The sAIMoveAction structure provides the uniform information
// structure through which components make instantaneous movement
// suggestions to the enactors.
//
// Suggestion memory must be dynamically allocated and is owned/freed by
// the move action class
//

enum eAIMoveActionFlags
{
   kAIAF_GoalStale = kAIAF_BaseFirstAvail
};

///////////////////////////////////////

class cAIMoveAction : public cAIAction
{
public:
   cAIMoveAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIMoveAction();

   //
   // Make a suggestion. Note memory ownership transferred
   //
   void Add(sAIMoveSuggestion *);

   //
   // Clear out suggestions, freeing memory. Note, does not affect tags.
   //
   void Clear();

   //
   // Clear out suggestions, handing suggestion memory back
   //
   void Detach(cAIMoveSuggestions *);

   //
   // Resolve suggestions into a move
   //
   // This is only necessary if you wish to see the
   // resolution and possibly develop a new plan as a result
   // Nest suggestion will be null if winning suggestion did not come
   // from the move action, i.e., was a move regulation. (toml 04-02-98)
   //
   BOOL NeedsResolution();
   BOOL Resolve(const sAIMoveSuggestion ** ppBestSuggestion = NULL);

   //
   // Access internals
   //
   const cAIMoveSuggestions & GetSuggestions() const;
   const sAIMoveGoal &        GetMoveGoal() const;

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

protected:
   // Cached pointer to move enactor
   IAIMoveEnactor *    m_pEnactor;

private:
   // The current move suggestions
   cAIMoveSuggestions  m_Suggestions;

   // Resolved move
   sAIMoveGoal         m_Goal;

   // Timer to enforce minimum interruptability rate
   cAITimer            m_MinInterrupt;
   
};

///////////////////////////////////////

inline cAIMoveAction::cAIMoveAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Move, pOwner, data),
   m_MinInterrupt(AICustomTime(5000))
{
   m_pEnactor = m_pAI->AccessMoveEnactor();
   flags |= kAIAF_GoalStale;
}

///////////////////////////////////////

inline void cAIMoveAction::Add(sAIMoveSuggestion * pSuggestion)
{
   m_Suggestions.Append(pSuggestion);
   flags |= kAIAF_GoalStale;
}

///////////////////////////////////////

inline void cAIMoveAction::Clear()
{
   for (int i = 0; i < m_Suggestions.Size(); i++)
      delete m_Suggestions[i];
   m_Suggestions.SetSize(0);
   flags |= kAIAF_GoalStale;
}

///////////////////////////////////////

inline const cAIMoveSuggestions & cAIMoveAction::GetSuggestions() const
{
   return m_Suggestions;
}

///////////////////////////////////////

inline const sAIMoveGoal & cAIMoveAction::GetMoveGoal() const
{
   return m_Goal;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTMOV_H */
