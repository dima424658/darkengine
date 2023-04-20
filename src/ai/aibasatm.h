/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasatm.h,v 1.2 2000/01/29 12:45:02 adurant Exp $
//
// Template file for non-inline members of the base implementation of an AI actor
//
#pragma once

#ifndef __AIBASATM_H
#define __AIBASATM_H

#include <aiapibhv.h>
#include <aibasact.h>
#include <aibasatm.h>
#include <aibasctm.h>
#include <aitagtyp.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cAIActorBase
//

#define AI_ACTOR_BASE_TEMPLATE  template <class INTERFACE, const GUID * pIID_INTERFACE>
#define AI_ACTOR_BASE           cAIActorBase<INTERFACE, pIID_INTERFACE>
#define AI_ACTOR_BASE_BASE      cAIComponentBase<INTERFACE, pIID_INTERFACE>

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
STDMETHODIMP AI_ACTOR_BASE::QueryInterface(REFIID id, void ** ppI)
{
   if (IsEqualGUID(id, IID_IAIActor))
   {
      this->AddRef();
      *ppI = (IAIActor *)this;
      return S_OK;
   }
   return AI_ACTOR_BASE_BASE::QueryInterface(id, ppI);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
STDMETHODIMP_(BOOL) AI_ACTOR_BASE::SaveActions(ITagFile * pTagFile, cAIActions *pActions)
{
   unsigned nActions = pActions->Size();
   AITagMove(pTagFile, &nActions);
   for (int i = 0; i < nActions; i++)
      this->m_pAI->AccessBehaviorSet()->SaveAction(pTagFile, (*pActions)[i]);
   return TRUE;
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
STDMETHODIMP_(BOOL) AI_ACTOR_BASE::LoadActions(ITagFile * pTagFile, cAIActions *pActions)
{
   unsigned nActions;

   AITagMove(pTagFile, &nActions);
   for (int i = 0; i < nActions; i++)
   {
      IAIAction *pAction = this->m_pAI->AccessBehaviorSet()->LoadAndCreateAction(pTagFile, this);
      if (pAction)
         pActions->Append(pAction);
   }      
   return TRUE;
}
   
///////////////////////////////////////
//
// Default action suggestor
//

AI_ACTOR_BASE_TEMPLATE  
STDMETHODIMP AI_ACTOR_BASE::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   return S_OK;
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIMoveAction * AI_ACTOR_BASE::CreateMoveAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateMoveAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAILocoAction * AI_ACTOR_BASE::CreateLocoAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateLocoAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIMotionAction * AI_ACTOR_BASE::CreateMotionAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateMotionAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAISoundAction * AI_ACTOR_BASE::CreateSoundAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateSoundAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIOrientAction * AI_ACTOR_BASE::CreateOrientAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateOrientAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIFrobAction * AI_ACTOR_BASE::CreateFrobAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateFrobAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIFollowAction * AI_ACTOR_BASE::CreateFollowAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateFollowAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIInvestAction * AI_ACTOR_BASE::CreateInvestAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateInvestAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIWanderAction * AI_ACTOR_BASE::CreateWanderAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateWanderAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIPsdScrAction * AI_ACTOR_BASE::CreatePsdScrAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreatePsdScrAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
cAIWaitAction * AI_ACTOR_BASE::CreateWaitAction(DWORD data)
{
   return this->m_pAI->AccessBehaviorSet()->CreateWaitAction(this, data);
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
BOOL AI_ACTOR_BASE::IsOwn(IAIAction * pAction) const
{
   return (pAction && (pAction->Access()->pOwner == (IAIActor *)this || pAction->GetTrueOwner() == (IAIActor *)this));
}
   
///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
BOOL AI_ACTOR_BASE::IsRelevant(IAIAction * pPrevious, IAIAction * pAction) const
{
   return ((pPrevious && pPrevious->Access()->pOwner == (IAIActor *)this) || (pAction && pAction->Access()->pOwner == (IAIActor *)this));
}
   
///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
BOOL AI_ACTOR_BASE::IsFirstAction(IAIAction * pPrevious, IAIAction * pAction) const
{
   return (!pPrevious || pPrevious->Access()->pOwner != (IAIActor *)this);
   return 0;
}

///////////////////////////////////////

AI_ACTOR_BASE_TEMPLATE  
HRESULT AI_ACTOR_BASE::ContinueActions(const cAIActions & previous, cAIActions *pNew) const
{
   int i;
   *pNew=previous;
   for (i=0; i<pNew->Size(); i++)
      (*pNew)[i]->AddRef();
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIBASATM_H */
