/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactmot.cpp,v 1.9 2000/02/11 18:27:15 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <aiapimov.h>
#include <aiactmot.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionAction
//

cAIMotionAction::~cAIMotionAction()
{
}

///////////////////////////////////////
//
// Update the action. Note that a movement action never "succeeds," but may
// fail and is interruptable
//

DECLARE_TIMER(AI_AIMTA_U, Average);

STDMETHODIMP_(eAIResult) cAIMotionAction::Update()
{
   if (!m_pEnactor)
   {
      Warning(("No movement enactor for motion action from AI %d\n", m_pAIState->GetID()));
      result = kAIR_Fail;
      return result;
   }

   // If we haven't already resolved...
   if (AIInProgress(result))
   {
      AUTO_TIMER(AI_AIMTA_U);

      cStr descStr;

      switch (m_pEnactor->GetStatus())
      {
         case kAIME_Idle:
         {
            if (Started())
            {
               result = kAIR_Success;
               break;
            }
            // else fall through...
         }
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

      if (AIInProgress(result) && m_Timeout.Expired())
      {
         Describe(&descStr);
         Warning(("Motion action timed out! %s\n", descStr.operator const char *()));
         result = kAIR_Fail;
         m_Timeout.Reset();
      }
   }

   return result;
}

///////////////////////////////////////
//
// Enact the action.
//

DECLARE_TIMER(AI_AIMTA_E, Average);

STDMETHODIMP_(eAIResult) cAIMotionAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(AI_AIMTA_E);

   if (!m_pEnactor)
   {
      Warning(("No movement enactor for motion action from AI %d\n", m_pAIState->GetID()));
      result = kAIR_Fail;
      return result;
   }

   if (!Started())
   {
      m_MinInterrupt.Reset();
      m_Timeout.Reset();
   }
   if (FAILED(m_pEnactor->EnactAction(this, deltaTime)))
      result = kAIR_Fail;
   if (!Started())
      SetStarted(TRUE);

   return result;
}

///////////////////////////////////////////////////////////////////////////////
