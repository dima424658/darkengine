/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactseq.h,v 1.3 2000/01/29 12:44:59 adurant Exp $
//
// Sequenced actions
//
#pragma once

#ifndef __AIACTSEQ_H
#define __AIACTSEQ_H

#include <lg.h>
#include <str.h>
#include <dynarray.h>

#include <aibasact.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISeqAction
//

class cAISeqAction : public cAIAction
{
public:
   cAISeqAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAISeqAction();

   // Count the actions in the sequence
   int GetCount() const;
   
   // Add to the sequence
   void Add(IAIAction * pAction);
   void Add(const cAIActions &);

   // Override the default description
   void SetDescription(const cStr &);

   // Describe the action, in detailed human terms
   STDMETHOD_(void, Describe)(cStr * pStr);

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:

   int        m_iCurrent;
   cAIActions m_sequence;
   cStr       m_description;
};

///////////////////////////////////////

inline cAISeqAction::cAISeqAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Sequence, pOwner, data),
   m_iCurrent(0)
{
}

///////////////////////////////////////

inline int cAISeqAction::GetCount() const
{
   return m_sequence.Size();
}

///////////////////////////////////////

inline void cAISeqAction::Add(IAIAction * pAction)
{
   m_sequence.Append(pAction);
   pAction->AddRef();
}

///////////////////////////////////////

inline void cAISeqAction::Add(const cAIActions & actions)
{
   for (int i = 0; i < actions.Size(); i++)
   {
      m_sequence.Append(actions[i]);
      actions[i]->AddRef();
   }
}

///////////////////////////////////////

inline void cAISeqAction::SetDescription(const cStr & description)
{
   m_description = description;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIACTSEQ_H */
