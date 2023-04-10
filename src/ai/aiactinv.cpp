/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactinv.cpp,v 1.13 2000/03/05 15:48:33 toml Exp $
//

// #define PROFILE_ON 1

#include <lg.h>

#include <ctagset.h>
#include <mtagvals.h>

#include <aiactinv.h>
#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactori.h>
#include <aiactwnd.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////


static cTagSet g_AISearchTags;
static cTagSet g_AIScanTags;

#define kAIDefScanDelayMin 6000
#define kAIDefScanDelayMax 18000

///////////////////////////////////////////////////////////////////////////////

cAIInvestAction::cAIInvestAction(IAIActor * pOwner, DWORD data)
 : cAIAction      (kAIAT_Investigate, pOwner, data),
   m_pLocoAction  (NULL),
   m_pMotionAction(NULL),
   m_pOrientAction(NULL),
   m_pWanderAction(NULL),
   m_StyleFlags   (kAIIS_EndWander),
   m_ScanTimer    (kAIDefScanDelayMin, kAIDefScanDelayMax),
   m_expiration   (AICustomTime(kAIIA_InfiniteDur)),
   m_CheckDistTimer(AICustomTime(1250)),
   fPathNearDist(-1),
   m_bDoCheckDist(TRUE)
{
   m_ScanTimer.Reset();

   // If this is the first investigation, set up our tags
   if (!g_AISearchTags.Size())
   {
      g_AISearchTags.Add(cTag("Search", kMTV_set));

      g_AIScanTags.Add(cTag("Search", kMTV_set));
      g_AIScanTags.Add(cTag("Scan", kMTV_set));
   }
}

///////////////////////////////////////

cAIInvestAction::~cAIInvestAction()
{
   SafeRelease(m_pOrientAction);
   SafeRelease(m_pLocoAction);
   SafeRelease(m_pMotionAction);
   SafeRelease(m_pWanderAction);
}

///////////////////////////////////////

void cAIInvestAction::Set(const cMxsVector & dest,
                          ObjID              hintObject,
                          eAISpeed           speed,
                          float              accuracySq,
                          float              accuracyZ)
{
   SafeRelease(m_pMotionAction);
   SafeRelease(m_pWanderAction);
   SafeRelease(m_pOrientAction);
   if (!m_pLocoAction)
      m_pLocoAction = CreateLocoAction();

   m_pLocoAction->SetPathNearDist(fPathNearDist);

   if (speed == kAIS_Num)
      m_StyleFlags |= kAIIS_VarSpeed;
   else
      m_StyleFlags &= ~kAIIS_VarSpeed;

   m_CheckDistTimer.Force();
   m_pLocoAction->Set(dest, hintObject, speed, accuracySq, accuracyZ);
   m_state = kEnRoute;
   result = kAIR_NoResult;
}

///////////////////////////////////////

void cAIInvestAction::Set(ObjID    object,
                          eAISpeed speed,
                          float    accuracySq,
                          float    accuracyZ)
{
   SafeRelease(m_pMotionAction);
   SafeRelease(m_pWanderAction);
   SafeRelease(m_pOrientAction);
   if (!m_pLocoAction)
      m_pLocoAction = CreateLocoAction();

   m_pLocoAction->SetPathNearDist(fPathNearDist);

   if (speed == kAIS_Num)
      m_StyleFlags |= kAIIS_VarSpeed;
   else
      m_StyleFlags &= ~kAIIS_VarSpeed;

   m_pLocoAction->Set(object, speed, accuracySq, accuracyZ);
   m_CheckDistTimer.Force();
   m_state = kEnRoute;
   result = kAIR_NoResultSwitch;
}

///////////////////////////////////////

void cAIInvestAction::Set(const cMxsVector & dest,
                          eAISpeed           speed,
                          float              accuracySq,
                          float              accuracyZ)
{
   SafeRelease(m_pMotionAction);
   SafeRelease(m_pWanderAction);
   SafeRelease(m_pOrientAction);
   if (!m_pLocoAction)
      m_pLocoAction = CreateLocoAction();

   m_pLocoAction->SetPathNearDist(fPathNearDist);

   if (speed == kAIS_Num)
   {
      speed = kAIS_Normal;
      m_StyleFlags |= kAIIS_VarSpeed;
   }
   else
      m_StyleFlags &= ~kAIIS_VarSpeed;

   m_pLocoAction->Set(dest, speed, accuracySq, accuracyZ);
   m_state = kEnRoute;
   result = kAIR_NoResultSwitch;
}

///////////////////////////////////////

void cAIInvestAction::SetScan(unsigned minScanDelay, unsigned maxScanDelay)
{
   m_ScanTimer.Set(minScanDelay, maxScanDelay);
}

///////////////////////////////////////

void cAIInvestAction::ResetScan()
{
   m_ScanTimer.Reset();
}

///////////////////////////////////////

void cAIInvestAction::ForceScan()
{
   m_ScanTimer.Force();
}

///////////////////////////////////////

const mxs_vector & cAIInvestAction::GetDest() const
{
   if (m_pLocoAction)
      return m_pLocoAction->GetLogicalDest();
   return kInvalidLoc;
}

///////////////////////////////////////

float cAIInvestAction::ComputePathDist()
{
   if (m_pLocoAction)
      return m_pLocoAction->ComputePathDist();
   return 0.0;
}

///////////////////////////////////////
//
// Update the action
//

STDMETHODIMP_(eAIResult) cAIInvestAction::Update()
{
   // If we've already resolved, we're done...
   if (!InProgress())
      return result;

   switch (m_state)
   {
      case kInitOrient:
      {
         Assert_(m_pOrientAction);
         result = m_pOrientAction->Update();

         if (!AIInProgress(result))
            result = kAIR_NoResultSwitch;

         break;
      }

      case kEnRoute:
      {
         AssertMsg(m_pLocoAction, "Investigate action used without Set()");
         result = m_pLocoAction->Update();

#if 0
         if (result == kAIR_Success) // propagate failure
#else
         if (!AIInProgress(result))
#endif
         {
            m_expiration.Reset();
            result = kAIR_NoResultSwitch;
         }
         break;
      }

      case kScanEnRoute:
      case kScanAtLoc:
      {
         if (m_pMotionAction)
         {
            result = m_pMotionAction->Update();
            if (!AIInProgress(result))
            {
               m_ScanTimer.Reset();
               result = kAIR_NoResultSwitch;
            }
         }
         else if (m_pWanderAction)
         {
            result = m_pWanderAction->Update();
            if (!AIInProgress(result))
            {
               result = kAIR_NoResultSwitch;
            }
         }
         break;
      }
   }

   return result;
}

///////////////////////////////////////
//
//
//

float g_AIDistanceStartSearch = 35.0;
float g_AIDistanceRun = 70.0;

DECLARE_TIMER(cAIInvestAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIInvestAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIInvestAction_Enact);

   cAIAction * pAction = NULL;

   if (!Started())
   {
      SetStarted(TRUE);

      if (m_StyleFlags & kAIIS_InitScan)
      {
         ForceScan();
         // but allow a bit of loco first
         m_ScanTimer.Delay(750);
      }

      m_state = (m_StyleFlags & kAIIS_InitOrient) ? kInitOrient : kEnRoute;
   }
   else if (!CanEnact())
      return result;

   switch (m_state)
   {
      case kInitOrient:
      {
         floatang angleTo = m_pAIState->AngleTo(m_pLocoAction->GetDest());
         if (!m_pOrientAction || m_pOrientAction->InProgress() || ffabsf(Delta(m_pAIState->GetFacingAng(), angleTo).value) > DEGREES(45))
         {
            if (!m_pOrientAction)
            {
               m_pOrientAction = CreateOrientAction();
               m_pOrientAction->Set(angleTo);
               m_pOrientAction->AddTags(g_AISearchTags);
            }
            pAction = m_pOrientAction;
            break;
         }
         else
         {
            SafeRelease(m_pOrientAction);
         }

         m_state = kEnRoute;
         // fall through...
      }

      case kEnRoute:
      {
         AssertMsg(m_pLocoAction, "Investigate action used without Set()");
         if (m_pLocoAction->InProgress())
         {
            if (!m_ScanTimer.Expired())
            {
               pAction = m_pLocoAction;
               break;
            }
            m_state = kScanEnRoute;
            Assert_(!m_pMotionAction);
         }
         else
            m_state = kScanAtLoc;
         // fall through...
      }
      case kScanEnRoute:
      case kScanAtLoc:
      {
         if (m_state == kScanAtLoc)
         {
            if (m_expiration.Expired())
            {
               SafeRelease(m_pMotionAction);
               SafeRelease(m_pWanderAction);
               m_state = kEnRoute;
               result = kAIR_Success;
               break;
            }
            else if (m_StyleFlags & kAIIS_EndWander)
            {
               if (!m_pWanderAction)
               {
                  m_pWanderAction = CreateWanderAction();
                  m_pWanderAction->AddTags(g_AISearchTags);
                  m_pWanderAction->SetPauseMotion(g_AIScanTags);
               }
               pAction = m_pWanderAction;
               break;
            }
         }

         if (m_state == kScanAtLoc || !m_pMotionAction || m_pMotionAction->InProgress())
         {
            if (m_pMotionAction && !m_pMotionAction->InProgress())
            {
               Assert_(m_state == kScanAtLoc);
               SafeRelease(m_pMotionAction);

            }
            if (!m_pMotionAction)
            {
               m_pMotionAction = CreateMotionAction();
               m_pMotionAction->AddTags(g_AIScanTags);
            }
            pAction = m_pMotionAction;
         }
         else
         {
            SafeRelease(m_pMotionAction);
            m_state = kEnRoute;
            pAction = m_pLocoAction;
            break;
         }
      }
   }

   if (m_state == kEnRoute && m_bDoCheckDist && m_CheckDistTimer.Expired())
   {
      m_pLocoAction->ClearTags();
      float pathDist = m_pLocoAction->ComputePathDist();
      if (pathDist < g_AIDistanceStartSearch)
      {
         m_pLocoAction->AddTags(g_AISearchTags);
         if (m_StyleFlags & kAIIS_VarSpeed)
         {
            m_pLocoAction->SetSpeed(kAIS_Normal);
         }
      }
      else
      {
         if (m_StyleFlags & kAIIS_VarSpeed)
         {
            if (pathDist > g_AIDistanceRun)
            {
               m_pLocoAction->SetSpeed(kAIS_VeryFast);
            }
            else
            {
               m_pLocoAction->SetSpeed(kAIS_Normal);
            }
         }

      }
      m_CheckDistTimer.Reset();
   }

   if (pAction)
      pAction->Enact(deltaTime);

   return result;
}

///////////////////////////////////////////////////////////////////////////////

