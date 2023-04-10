/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactprx.cpp,v 1.4 2000/02/19 12:17:19 toml Exp $
//
//
//

// #define PROFILE_ON 1

#include <aiactprx.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProxAction
//

cAIProxAction::cAIProxAction(IAIAction * pInner, IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Proxy, pOwner, data),
   m_pInner(pInner)
{
   m_pInner->AddRef();
}

///////////////////////////////////////

cAIProxAction::~cAIProxAction()
{
   m_pInner->Release();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIProxAction::Describe(cStr * pStr)
{
   m_pInner->Describe(pStr);
}

///////////////////////////////////////
//
// Access the internals of the action
//

STDMETHODIMP_(BOOL) cAIProxAction::InProgress()
{
   return m_pInner->InProgress();
}

///////////////////////////////////////

STDMETHODIMP_(tAIActionType) cAIProxAction::GetType()
{
   return type;
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIProxAction::GetResult()
{
   return m_pInner->GetResult();

///////////////////////////////////////
}

STDMETHODIMP_(DWORD) cAIProxAction::GetData(unsigned index)
{
   return ownerDataSet[index];
}

///////////////////////////////////////

STDMETHODIMP_(const sAIAction *) cAIProxAction::Access()
{
   return this;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIProxAction::IsProxy()
{
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(IAIAction *) cAIProxAction::GetInnerAction()
{
   return m_pInner;
}

///////////////////////////////////////

STDMETHODIMP_(IAIAction *) cAIProxAction::GetTrueAction()
{
   return m_pInner->GetTrueAction();
}

///////////////////////////////////////

STDMETHODIMP_(IAIActor *) cAIProxAction::GetTrueOwner()
{
   return m_pInner->GetTrueOwner();
}

///////////////////////////////////////
//
// Update the action
//

STDMETHODIMP_(eAIResult) cAIProxAction::Update()
{
   return m_pInner->Update();
}

///////////////////////////////////////
//
//
//

DECLARE_TIMER(cAIProxAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIProxAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIProxAction_Enact);

   return m_pInner->Enact(deltaTime);
}

///////////////////////////////////////
//
// Terminate the action
//

STDMETHODIMP_(eAIResult) cAIProxAction::End()
{
   return m_pInner->End();
}

///////////////////////////////////////////////////////////////////////////////
