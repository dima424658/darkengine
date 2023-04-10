/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitrig.cpp,v 1.3 2000/02/11 18:28:02 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <dynarray.h>

#include <aiactps.h>
#include <aigoal.h>
#include <aipsdscr.h>
#include <aitagtyp.h>
#include <aitrig.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

struct sAIPsdScrActs
{
   eAIPriority  priority;
   unsigned     n;
   sAIPsdScrAct acts[1];
};

///////////////////////////////////////

inline sAIPsdScrActs * NewActs(eAIPriority priority, const sAIPsdScrAct * pActs, unsigned n)
{
   sAIPsdScrActs * p = (sAIPsdScrActs *)malloc(sizeof(sAIPsdScrActs) + (sizeof(sAIPsdScrAct) * (n - 1)));
   p->priority = priority;
   p->n        = n;
   memcpy(p->acts, pActs, n * sizeof(sAIPsdScrAct));
   return p;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITriggeredPScripted
//

#define kAIListenRate kAIT_2Hz

cAITriggeredPScripted::cAITriggeredPScripted(eAITimerPeriod testRate, unsigned SaveLoadSubtag)
 : m_Timer(testRate),
   m_pActs(NULL),
   m_SaveLoadSubtag(SaveLoadSubtag)
{
}

///////////////////////////////////////

cAITriggeredPScripted::~cAITriggeredPScripted()
{
   if (m_pActs)
      delete m_pActs;
}

///////////////////////////////////////

void cAITriggeredPScripted::SetTriggered(eAIPriority priority, const sAIPsdScrAct * pActs, unsigned nActs)
{
   if (m_pActs)
      delete m_pActs;
   m_pActs = NewActs(priority, pActs, nActs);
}

///////////////////////////////////////

void cAITriggeredPScripted::ClearTrigger()
{
   if (m_pActs)
      delete m_pActs;
   m_pActs = NULL;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAITriggeredPScripted::Init()
{
   SetNotifications(kAICN_ActionProgress);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAITriggeredPScripted::Save(ITagFile * pTagFile)
{
   if (m_SaveLoadSubtag && AIOpenTagBlock(GetID(), m_SaveLoadSubtag, 1, 0, pTagFile))
   {
      BaseAbilitySave(pTagFile);
      m_Timer.Save(pTagFile);

      BOOL fActs = (m_pActs && m_pActs->n);
      AITagMove(pTagFile, &fActs);
      if (fActs)
      {
         AITagMove(pTagFile, &m_pActs->priority);
         AITagMove(pTagFile, &m_pActs->n);
         AITagMoveRaw(pTagFile, &m_pActs->acts[0], sizeof(sAIPsdScrAct) * m_pActs->n);
      }
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAITriggeredPScripted::Load(ITagFile * pTagFile)
{
   if (m_SaveLoadSubtag && AIOpenTagBlock(GetID(), m_SaveLoadSubtag, 1, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      m_Timer.Load(pTagFile);
      BOOL fActs;
      AITagMove(pTagFile, &fActs);
      if (fActs)
      {
         sAIPsdScrActs tempActs;
         AITagMove(pTagFile, &tempActs.priority);
         AITagMove(pTagFile, &tempActs.n);

         m_pActs = (sAIPsdScrActs *)malloc(sizeof(sAIPsdScrActs) + (sizeof(sAIPsdScrAct) * (tempActs.n - 1)));
         *m_pActs = tempActs;
         AITagMoveRaw(pTagFile, &m_pActs->acts[0], sizeof(sAIPsdScrAct) * m_pActs->n);
      }
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAITriggeredPScripted::SaveGoal(ITagFile * pTagFile, cAIGoal * pGoal)
{
   if (!cAIAbility::SaveGoal(pTagFile, pGoal))
      return FALSE;

   if (!pGoal)
      return TRUE;

   Assert_(pGoal->ownerData);
   sAIPsdScrActs * pActs = (sAIPsdScrActs *)pGoal->ownerData;

   AITagMove(pTagFile, &pActs->priority);
   AITagMove(pTagFile, &pActs->n);
   AITagMoveRaw(pTagFile, &pActs->acts[0], sizeof(sAIPsdScrAct) * pActs->n);

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAITriggeredPScripted::LoadGoal(ITagFile * pTagFile, cAIGoal ** ppGoal)
{
   if (!cAIAbility::LoadGoal(pTagFile, ppGoal))
      return FALSE;

   if (!*ppGoal)
      return TRUE;

   sAIPsdScrActs * pActs;
   sAIPsdScrActs tempActs;

   AITagMove(pTagFile, &tempActs.priority);
   AITagMove(pTagFile, &tempActs.n);

   pActs = (sAIPsdScrActs *)malloc(sizeof(sAIPsdScrActs) + (sizeof(sAIPsdScrAct) * (tempActs.n - 1)));
   *pActs = tempActs;
   AITagMoveRaw(pTagFile, &pActs->acts[0], sizeof(sAIPsdScrAct) * pActs->n);

   (*ppGoal)->ownerData = (DWORD)pActs;
   (*ppGoal)->flags |= kAIGF_FreeData;

   return TRUE;
}

///////////////////////////////////////

DECLARE_TIMER(cAITriggeredPScripted_OnActionProgress, Average);

STDMETHODIMP_(void) cAITriggeredPScripted::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAITriggeredPScripted_OnActionProgress);

   if (IsOwn(pAction) && !pAction->InProgress())
      SignalGoal();
   else if (m_pActs && pAction->GetResult() > kAIR_NoResult && m_Timer.Expired())
   {
      m_Timer.Reset();
      SignalGoal();
   }
}

///////////////////////////////////////

STDMETHODIMP cAITriggeredPScripted::SuggestGoal(cAIGoal * pCurrentGoal, cAIGoal ** ppGoal)
{
   if (!m_pActs)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }

   *ppGoal = new cAICustomGoal(this);
   if (!m_pActs->priority)
      (*ppGoal)->priority = kAIP_Normal;
   else
      (*ppGoal)->priority = m_pActs->priority;

   (*ppGoal)->ownerData = (DWORD)m_pActs;
   (*ppGoal)->flags |= kAIGF_FreeData;
   m_pActs = NULL;

   SignalAction();

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAITriggeredPScripted::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   sAIPsdScrActs * pActs = (sAIPsdScrActs *)pGoal->ownerData;
   cAIPsdScrAction * pAction = CreatePsdScrAction();
   pAction->Set(pActs->acts, pActs->n);
   pNew->Append(pAction);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
