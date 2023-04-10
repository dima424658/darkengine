/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactmov.cpp,v 1.10 2000/02/11 18:27:16 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <aiapimov.h>
#include <aiactmov.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveAction
//

cAIMoveAction::~cAIMoveAction()
{
   Clear();
}

///////////////////////////////////////

BOOL cAIMoveAction::NeedsResolution()
{
   return ((flags & kAIAF_GoalStale) || m_pEnactor->NewRegulations());
}

///////////////////////////////////////

BOOL cAIMoveAction::Resolve(const sAIMoveSuggestion ** ppBestSuggestion)
{
   if (!m_pEnactor)
   {
      Warning(("No movement enactor for move action from AI %d\n", m_pAIState->GetID()));
      return FALSE;
   }

   cAIMoveSuggestions compositeSuggestions;

   compositeSuggestions.Add(m_Suggestions);
   m_pEnactor->AddMovementRegulations(compositeSuggestions);

   if (m_pEnactor->ResolveSuggestions(compositeSuggestions, ppBestSuggestion, &m_Goal) == S_OK)
   {
      flags &= ~kAIAF_GoalStale;
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////
//
// Update the action. Note that a movement action never "succeeds," but may
// fail and is interruptable
//

DECLARE_TIMER(AI_AIMVA_U, Average);

STDMETHODIMP_(eAIResult) cAIMoveAction::Update()
{
   if (!m_pEnactor)
   {
      Warning(("No movement enactor for move action from AI %d\n", m_pAIState->GetID()));
      result = kAIR_Fail;
      return result;
   }

   AssertMsg(m_pEnactor, "No movement enactor for move action");

   // If we haven't already resolved...
   if (InProgress())
   {
      AUTO_TIMER(AI_AIMVA_U);

      cStr descStr;

      switch (m_pEnactor->GetStatus())
      {
         case kAIME_Idle:
         case kAIME_ActiveInterrupt:
         {
            result = kAIR_NoResultSwitch;
            m_MinInterrupt.Reset();
            break;
         }

         case kAIME_ActiveBusy:
         {
            if (!m_MinInterrupt.Expired())
               result = kAIR_NoResult;
            else
            {
               result = kAIR_NoResultSwitch;
               m_MinInterrupt.Reset();
            }
            break;
         }

         default:
         {
            result = kAIR_Fail;
            Describe(&descStr);
            Warning(("Action failed: %s\n", descStr.operator const char *()));
         }
      }
   }

   return result;
}

///////////////////////////////////////
//
// Enact the action.
//

DECLARE_TIMER(AI_AIMVA_E, Average);

STDMETHODIMP_(eAIResult) cAIMoveAction::Enact(ulong deltaTime)
{
   if (!m_pEnactor)
   {
      Warning(("No movement enactor for move action from AI %d\n", m_pAIState->GetID()));
      result = kAIR_Fail;
      return result;
   }

   AUTO_TIMER(AI_AIMVA_E);

   if (!Started())
   {
      SetStarted(TRUE);
      m_MinInterrupt.Reset();
   }
   else if (!CanEnact())
      return result;

   if (NeedsResolution())
      Resolve();

   if (FAILED(m_pEnactor->EnactAction(this, deltaTime)))
      result = kAIR_Fail;

   return result;
}

///////////////////////////////////////////////////////////////////////////////
