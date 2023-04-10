/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactori.cpp,v 1.7 2000/02/11 18:27:19 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <aiactori.h>
#include <aiactmov.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIOrientAction
//

cAIOrientAction::~cAIOrientAction()
{
   SafeRelease(m_pMoveAction);
}

///////////////////////////////////////

#define kOrientEps 0.02

STDMETHODIMP_(eAIResult) cAIOrientAction::Update()
{
   if (!m_pMoveAction)
      result = kAIR_NoResultSwitch;
   else
   {
      floatarc arc;
      floatang curAngle = m_pAIState->GetFacingAng();
      arc.SetByCenterAndSpan(m_angle, DEGREES(10));

      if (arc.contains(curAngle) || (ffabsf(m_lastAngle.value - curAngle.value) < kOrientEps && m_Timeout.Expired()))
         result = kAIR_Success;

      else
      {
         if (ffabsf(m_lastAngle.value - curAngle.value) >= kOrientEps)
         {
            m_Timeout.Reset();
            m_lastAngle = curAngle;
         }
         result = m_pMoveAction->Update();
      }
   }

   return result;
}

///////////////////////////////////////

DECLARE_TIMER(cAIOrientAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIOrientAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIOrientAction_Enact);

   if (!Started())
   {
      SetStarted(TRUE);
      m_pMoveAction = CreateMoveAction();
      m_lastAngle = m_pAIState->GetFacingAng();
      switch (m_type)
      {
         case kAIOT_Absolute:
            break;

         case kAIOT_Toward:
         {
            mxs_vector objLoc;
            GetObjLocation(m_object, &objLoc);
            m_angle = m_pAIState->AngleTo(objLoc);
            break;
         }

         case kAIOT_Relative:
         {
            floatang objFacing;
            GetObjFacing(m_object, &objFacing);
            m_angle = objFacing - m_angle;
            break;
         }
      }

      sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

      pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
      pSuggestion->speed = kAIS_Stopped;
      pSuggestion->dirArc.SetByCenterAndSpan(m_angle, DEGREES(1.0));

      m_pMoveAction->Add(pSuggestion);
      m_pMoveAction->ClearTags();
      m_pMoveAction->AddTags(GetTags());

      m_Timeout.Reset();
   }
   else if (!CanEnact())
      return result;

   result = m_pMoveAction->Enact(deltaTime);

   return result;
}

///////////////////////////////////////////////////////////////////////////////
