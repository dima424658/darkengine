/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiproxy.cpp,v 1.4 2000/02/19 12:45:18 toml Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>

#include <playtest.h>   // for hello_debugger

#include <aitype.h>
#include <aidebug.h>
#include <aiproxy.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// Debugging globals
//

extern ObjID g_AIBreakID;

#define AIBreakIfSet() if (GetID() != g_AIBreakID) ; else { hello_debugger(); g_AIBreakID = OBJ_NULL; }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProxy.  For multi-player games, where the "brains" of this AI are elsewhere.
//

STDMETHODIMP_(BOOL) cAIProxy::IsNetworkProxy()
{
   return TRUE;
}

///////////////////////////////////////

// Proxies don't do anything that has to do with making decisions.  They only carry out actions.
HRESULT cAIProxy::OnNormalFrame()
{
   AIDebugSet(GetID());
   AIBreakIfSet();

   // Synchronize state structure with world database
   UpdateState();

   // Check if the AI should cycle...
   if (m_state.GetFlags() & (kAI_IsRemote | kAI_OutOfWorld))
      return S_FALSE;

   if (m_state.GetMode() == kAIM_Asleep || m_state.GetMode() == kAIM_Dead)
      return S_OK;

   NotifyBeginFrame();

   // For proxies, we still need to update senses so that queries about
   // knowledge by other AIs still works
   if (UpdateSenses()==S_OK)
      UpdateAlertness();     // determine alertness from the sense data....

   eAIResult result = UpdateAction();

   if (result > kAIR_NoResult && m_state.GetMode() == kAIM_Dead)
         return S_OK;

   if (m_pCurAbilityInfo && m_pCurAbilityInfo->CheckSignals(kAI_SigAct))
   {
      // We are in control of the actions.  Decide the next one.
      // Since we are a proxy, we never decide anything higher than actions.
      DecideAction();
      *m_pCurAbilityInfo->pSignals = 0;
   } else if (!m_pCurAbilityInfo) {
      // @HACK: This is a proxy without any abilities -- ugly, but such it is.
      // So we may need to clear out the action list ourselves, instead of
      // having DecideAction() do it for us.
      // In the long run, this should all get replaced by some sort of
      // "proxy ability", which can deal with this sort of stuff in the
      // usual way.
      if (result > kAIR_NoResultSwitch)
      {
         int i;
         for (i = 0; i < m_CurActions.Size(); i++)
         {
            m_CurActions[i]->Release();
         }
         m_CurActions.SetSize(0);
      }
   }
   Enact();

   return S_OK;
}

///////////////////////////////////////

DECLARE_TIMER(cAIProxy_Enact, Average);

// Like cAI::Enact, but without m_pMoveEnactor->NoAction() if no other motion is running.
void cAIProxy::Enact()
{
   AUTO_TIMER(cAIProxy_Enact);

   ulong deltaTime = AIGetFrameTime();

   for (int i = 0; i < m_CurActions.Size(); i++)
   {
      if (m_CurActions[i]->InProgress())
         m_CurActions[i]->Enact(deltaTime);
   }
}

///////////////////////////////////////

void cAIProxy::StartProxyAction(IAIAction *pAction, ulong deltaTime)
{
   // Stop any existing actions of the same type.
   StopActionOfType(pAction->GetType());

   // Start this one
   m_CurActions.Append(pAction);
   pAction->Enact(deltaTime);
}

///////////////////////////////////////////////////////////////////////////////
