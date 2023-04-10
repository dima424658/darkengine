/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactwnd.cpp,v 1.7 2000/02/11 18:27:24 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactwnd.h>
#include <aipathdb.h>
#include <aipthcst.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////

cAIWanderAction::cAIWanderAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Wander, pOwner, data)
{
   m_Center = kInvalidLoc;
   m_MinRadius = 7.0;
   m_MaxRadius = 25.0;
   m_Timer.Set(AICustomTime(kAIWA_InfiniteDur));
   m_OddsRecenter = 25;
   m_MinFrontClearance = 6.0;
   m_pLocoAction = 0;
   m_pMotionAction = 0;
   m_OddsMotion = 33;
   m_fLastWasCenter = TRUE;
}

///////////////////////////////////////

cAIWanderAction::~cAIWanderAction()
{
   SafeRelease(m_pLocoAction);
   SafeRelease(m_pMotionAction);
}

///////////////////////////////////////

cAILocoAction * cAIWanderAction::PickLoco()
{
   mxs_vector         target;
   const mxs_vector & aiLoc = *m_pAIState->GetLocation();
   IAIPathfinder *    pPathfinder = m_pAI->AccessPathfinder();
   tAIPathCellID      destCell = 0;

   if (!pPathfinder)
      return NULL;

   if (!m_fLastWasCenter && AIRandom(0, 99) < m_OddsRecenter)
   {
      target = m_Center;
      m_fLastWasCenter = TRUE;
      // mprintf("Returning to center\n");
   }
   else
   {
      BOOL     found = FALSE;
      float    distance;
      floatang angle;
      floatang angToAI;

      angToAI.set(m_Center.x, m_Center.y, aiLoc.x, aiLoc.y);

      for (int i = 0; i < 8; i++)
      {
         distance = ((m_MaxRadius - m_MinRadius) * (AIRandom(1, 100) / 100.0)) + m_MinRadius;
         if (i % 2)
            angle.value = angToAI.value + (DEGREES(AIRandom(30, 60)) * ((Rand() % 2) ? -1 : 1));
         else
            angle.value = angToAI.value + (DEGREES(AIRandom(210, 240)) * ((Rand() % 2) ? -1 : 1));
         floatang_normalize_angle(angle.value);

         // mprintf("Trying %d degrees, %d feet\n", (int)RADIANS(angle.value), (int)distance);

         ProjectFromLocationOnZPlane(aiLoc, distance + m_MinFrontClearance, angle, &target);

         if (pPathfinder->Pathcast(target))
         {
            // mprintf("Looks good\n");
            ProjectFromLocationOnZPlane(aiLoc, distance, angle, &target);
            pPathfinder->Pathcast(target, &destCell);
            Assert_(destCell);
            target.z = g_AIPathDB.GetZAtXY(destCell, target) + 3.0;

            m_fLastWasCenter = FALSE;
            found = TRUE;
            break;
         }
      }

      if (!found)
      {
         // mprintf("nothing good found, going back to center\n");
         target = m_Center;
         m_fLastWasCenter = TRUE;
      }
   }

   cAILocoAction * pAction = CreateLocoAction();
   pAction->Set(target);
   pAction->AddTags(tags);

   return pAction;
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIWanderAction::Update()
{
   // If we've already resolved, we're done...
   if (!InProgress())
      return result;
   if (m_pLocoAction)
   {
      result = m_pLocoAction->Update();

      if (!AIInProgress(result))
      {
         result = kAIR_NoResultSwitch;
         SafeRelease(m_pLocoAction);

         if (m_PauseMotionTags.Size() && AIRandom(0, 99) < m_OddsMotion)
         {
            m_pMotionAction = CreateMotionAction();
            m_pMotionAction->AddTags(m_PauseMotionTags);
         }
         else
         {
            m_pLocoAction = PickLoco();
         }
      }
   }
   else if (m_pMotionAction)
   {
      result = m_pMotionAction->Update();

      if (!AIInProgress(result))
      {
         result = kAIR_NoResultSwitch;
         SafeRelease(m_pMotionAction);
         m_pLocoAction = PickLoco();
      }
   }
   else
      result = kAIR_NoResultSwitch;

   if (result == kAIR_NoResultSwitch && m_Timer.Expired())
   {
      SafeRelease(m_pLocoAction);
      SafeRelease(m_pMotionAction);
      result = kAIR_Success;
   }

   return result;
}

///////////////////////////////////////

DECLARE_TIMER(cAIWanderAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIWanderAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIWanderAction_Enact);

   if (!Started())
   {
      SafeRelease(m_pLocoAction);
      SafeRelease(m_pMotionAction);

      if (kInvalidLoc == m_Center)
      {
         m_Center = *m_pAIState->GetLocation();
         m_fLastWasCenter = TRUE;
      }
      else
         m_fLastWasCenter = FALSE;

      m_pLocoAction = PickLoco();
      m_Timer.Reset();
      SetStarted(TRUE);
   }
   else if (!CanEnact())
      return result;

   if (m_pLocoAction)
      m_pLocoAction->Enact(deltaTime);
   else if (m_pMotionAction)
      m_pMotionAction->Enact(deltaTime);

   return kAIR_NoResult;
}

///////////////////////////////////////////////////////////////////////////////
