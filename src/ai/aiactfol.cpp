/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactfol.cpp,v 1.7 2000/02/11 18:27:07 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <aiactfol.h>
#include <aiactmov.h>
#include <aipthcst.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static void CalcTarget(const mxs_vector & origin,
                       floatang facing,
                       const mxs_vector & velocity,
                       float time,
                       floatang angle, float distance,
                       mxs_vector * pResult)
{
   floatang targ = facing - angle;
   pResult->z = origin.z;
   pResult->x = origin.x + (distance * cos(targ.value));
   pResult->y = origin.y + (distance * sin(targ.value));
   pResult->x += velocity.x * time;
   pResult->y += velocity.y * time;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFollowAction
//

cAIFollowAction::~cAIFollowAction()
{
   SafeRelease(m_pMoveAction);
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIFollowAction::Update()
{
   if (!m_pMoveAction)
      result = kAIR_NoResultSwitch;
   else
      result = m_pMoveAction->Update();

   return result;
}

///////////////////////////////////////

DECLARE_TIMER(cAIFollowAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIFollowAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIFollowAction_Enact);

   if (!Started())
   {
      SetStarted(TRUE);
      m_pMoveAction = CreateMoveAction();
      result = kAIR_NoResultSwitch;
      m_timer.Force();
   }
   else if (!CanEnact())
      return result;

   if (result == kAIR_NoResultSwitch && m_timer.Expired())
   {
      float      distSq;
      float      bestSq = 999999.0;
      mxs_vector followObjLoc;
      mxs_vector followObjVel;
      mxs_vector currentLoc;
      mxs_vector targetLoc;
      floatang   followObjFacing;
      eAISpeed   targetSpeed = kAIS_Normal; // @TBD (toml 08-04-98)

      GetObjLocation(m_object, &followObjLoc);
      GetObjVelocity(m_object, &followObjVel);
      GetObjFacing(m_object, &followObjFacing);

      for (int i = 0; i < m_angles.Size(); i++)
      {
         CalcTarget(followObjLoc, followObjFacing, followObjVel, (float)kAIF_UpdateTime / 1000.0, m_angles[i], m_distances[i], &currentLoc);
         distSq = m_pAIState->DistXYSq(currentLoc);
         if (distSq < bestSq && m_pAI->AccessPathfinder()->Pathcast(currentLoc))
         {
            bestSq    = distSq;
            targetLoc = currentLoc;
         }
      }

      if (bestSq == 999999.0)
      {
         // Warning(("Follow (%d) is really at a loss. Should pathfind.\n", m_pAIState->GetID()));
         targetLoc = followObjLoc;
      }

      m_pMoveAction->Clear();

      sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

      if (m_pAIState->DistSq(targetLoc) < sq(2.0)) // @TBD (toml 08-04-98): fix this magic number
      {
         pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
         pSuggestion->speed = kAIS_Stopped;
         pSuggestion->dirArc.SetByCenterAndSpan(followObjFacing, DEGREES(1.0));
      }
      else
      {
         pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
         pSuggestion->dirArc.SetByCenterAndSpan(m_pAIState->AngleTo(targetLoc),
                                                floatang(PI*1.8));
         pSuggestion->speed       = targetSpeed;
         pSuggestion->facing.type = kAIF_SpecificDir;
         pSuggestion->facing.ang  = followObjFacing.value;
         pSuggestion->dest        = targetLoc;
      }

      m_pMoveAction->Add(pSuggestion);
   }

   result = m_pMoveAction->Enact(deltaTime);

   return result;
}

///////////////////////////////////////////////////////////////////////////////
