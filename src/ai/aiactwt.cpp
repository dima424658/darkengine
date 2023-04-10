/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactwt.cpp,v 1.5 2000/02/11 18:27:25 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <aiactmot.h>
#include <aiactwt.h>
#include <aitagtyp.h>
#include <aiapibhv.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWaitAction
//

cAIWaitAction::~cAIWaitAction()
{
   SafeRelease(m_pMotion);
}

///////////////////////////////////////
//
// Update the action.
//

STDMETHODIMP_(eAIResult) cAIWaitAction::Update()
{
   // If we've already resolved, we're done...
   if (!InProgress())
      return result;

   // If playing a motion, right now the behavior is for wait to go to the
   // completion of the current iteration
   if (m_pMotion)
   {
      result = m_pMotion->Update();
      if (AIInProgress(result))
         return result;

      result = kAIR_NoResultSwitch;
      m_pMotion->Restart();
   }
   else if (m_Timer.Expired())
   {
      m_Timer.Reset();
      result = kAIR_NoResultSwitch;
   }
   else
      result = kAIR_NoResult;

   if (result == kAIR_NoResultSwitch && AIGetTime() > m_TimeDone)
   {
      result = kAIR_Success;
   }

   return result;
}

///////////////////////////////////////
//
// Start the action
//

DECLARE_TIMER(cAIWaitAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIWaitAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIWaitAction_Enact);

   if (!Started())
   {
      SetStarted(TRUE);
      if (tags.Size())
      {
         m_pMotion = CreateMotionAction();
         m_pMotion->AddTags(tags);
      }
   }
   else if (!CanEnact() || !AIInProgress(result))
      return result;

   if (m_pMotion)
   {
      result = m_pMotion->Enact(deltaTime);
      if (!AIInProgress(result))
         result = kAIR_NoResultSwitch;
   }

   return result;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIWaitAction::Save(ITagFile * pTagFile)
{
   size_t versionSize = sizeof(*this);  // Rough version checking.
   AITagMove(pTagFile, &versionSize);
   SaveActionBase(pTagFile);
   AITagMove(pTagFile, &m_TimeDone);
   BOOL hasMotion = !!m_pMotion;
   AITagMove(pTagFile, &hasMotion);
   if (hasMotion)
      m_pAI->AccessBehaviorSet()->SaveAction(pTagFile, m_pMotion);
   m_Timer.Save(pTagFile);
   return TRUE;
}

///////////////////////////////////////


STDMETHODIMP_(BOOL) cAIWaitAction::Load(ITagFile * pTagFile)
{
   size_t versionSize;  // Rough version checking.
   AITagMove(pTagFile, &versionSize);
   AssertMsg(versionSize == sizeof(*this), "Invalid saved pseudo action encountered");
   LoadActionBase(pTagFile);
   AITagMove(pTagFile, &m_TimeDone);
   BOOL hasMotion;
   AITagMove(pTagFile, &hasMotion);
   if (hasMotion)
      m_pMotion = (cAIMotionAction *)m_pAI->AccessBehaviorSet()->LoadAndCreateAction(pTagFile, pOwner);
   m_Timer.Load(pTagFile);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
