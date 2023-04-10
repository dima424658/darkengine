/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactseq.cpp,v 1.4 2000/02/11 18:27:22 bfarquha Exp $
//
// Sequenced actions
//

// #define PROFILE_ON 1

#include <aitype.h>
#include <aidebug.h>
#include <aiactseq.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISeqAction
//

cAISeqAction::~cAISeqAction()
{
   for (int i = 0; i < m_sequence.Size(); i++)
      m_sequence[i]->Release();
}

///////////////////////////////////////
//
// Describe the action, in detailed human terms
//

STDMETHODIMP_(void) cAISeqAction::Describe(cStr * pStr)
{
   if (m_description.GetLength())
      *pStr = m_description;
   else
   {
      // @TBD (toml 05-18-98):  should build a default composite description here
      cAIAction::Describe(pStr);
   }
}

///////////////////////////////////////
//
// Update the action.
//

STDMETHODIMP_(eAIResult) cAISeqAction::Update()
{
   // If we haven't already resolved...
   if (InProgress())
   {
      // If we're not doing anything, there's nothing to update
      if (!m_sequence.Size())
         result = kAIR_NoResultSwitch;

      // Otherwise, check progress...
      else
      {
         result = m_sequence[m_iCurrent]->Update();

         // If the sequence step is complete, advance the sequence, possibly done
         if (result == kAIR_Success)
         {
            m_iCurrent++;
            if (m_iCurrent < m_sequence.Size())
            {
               AIWatch(Flow, m_pAIState->GetID(), "Sequenced action step complete");
#ifndef SHIP
               if (AIIsWatched(Flow, m_pAIState->GetID()))
               {
                  cStr desc;
                  const char * pszAbilityName;
                  if (m_sequence[m_iCurrent])
                  {
                     m_sequence[m_iCurrent]->Describe(&desc);
                     pszAbilityName = pOwner->GetName();
                  }
                  else
                  {
                     desc = "(No action)";
                     pszAbilityName = "(none)";
                  }

                  AIWatch2(Flow, m_pAIState->GetID(), "Sequenced action: \"%s\" from ability \"%s\"", desc.operator const char *(), pszAbilityName);
               }
#endif
               result = kAIR_NoResultSwitch;
            }
            else
               AIWatch(Flow, m_pAIState->GetID(), "Sequenced action done");
         }
         else
            AIWatchTrue(result > kAIR_Success, Flow, m_pAIState->GetID(), "Sequenced action step failed");
      }
   }

   if (result > kAIR_NoResultSwitch)
      // We're done, or there was an error
      pctComplete = 100;

   return result;
}

///////////////////////////////////////
//
// Start the action.
//

DECLARE_TIMER(cAISeqAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAISeqAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAISeqAction_Enact);

   if (!Started())
   {
      SetStarted(TRUE);
#ifndef SHIP
      if (AIIsWatched(Flow, m_pAIState->GetID()))
      {
         cStr desc;
         const char * pszAbilityName;
         if (m_sequence[m_iCurrent])
         {
            m_sequence[m_iCurrent]->Describe(&desc);
            pszAbilityName = pOwner->GetName();
         }
         else
         {
            desc = "(No action)";
            pszAbilityName = "(none)";
         }

         AIWatch2(Flow, m_pAIState->GetID(), "Sequenced action: \"%s\" from ability \"%s\"", desc.operator const char *(), pszAbilityName);
      }
#endif
   }

   result = m_sequence[m_iCurrent]->Enact(deltaTime);

   if (result == kAIR_Success && m_iCurrent < m_sequence.Size())
      result = kAIR_NoResultSwitch;

   return kAIR_NoResult;
}

///////////////////////////////////////////////////////////////////////////////
