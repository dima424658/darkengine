/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactloc.cpp,v 1.38 2000/03/21 12:24:08 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <objpos.h>

#include <aiapipth.h>
#include <aiactloc.h>
#include <aiactmov.h>
#include <aiapibhv.h>
#include <aidebug.h>
#include <aidist.h>
#include <aigoal.h>
#include <aiokbits.h>
#include <aipthedg.h>
#include <aipthloc.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILocoAction
//

cAILocoAction::~cAILocoAction()
{
   SafeRelease(m_pPath);

   if (m_pMoveAction)
      m_pMoveAction->Clear();
   SafeRelease(m_pMoveAction);
}

///////////////////////////////////////
//
// Check progress
//

eAIResult cAILocoAction::UpdatePathProgress()
{
   if (result > kAIR_NoResultSwitch)
      return result;

   BOOL fReached = FALSE;

   // Accuracy sphere
   if (m_accuracyZ == kFloatMax)
   {
      fReached = (m_pAIState->DistSq(m_dest) < m_accuracySq);
   }
   // Accuracy cylinder
   else if (m_pAIState->DiffZ(m_dest) < m_accuracyZ + m_pAI->GetGroundOffset())
   {
      fReached = (m_pAIState->DistXYSq(m_dest) < m_accuracySq);
   }

   if (fReached)
   {
      // ...the locomotion action is complete
      pctComplete = 100;
      return kAIR_Success;
   }

   pctComplete = 0;                              // @TBD (toml 04-03-98): have path store original edge count, then use current over that to give pctComplete

   return kAIR_NoResult;
}

///////////////////////////////////////
//
// Clear out our path and action info
//

void cAILocoAction::ClearPathAndMove()
{
   SafeRelease(m_pPath);

   if (m_pMoveAction)
      m_pMoveAction->Clear();

   SetStarted(FALSE);
   result = kAIR_NoResult;
   flags &= ~kAIAF_HeadingDirect;
   pctComplete = 0;
}

///////////////////////////////////////
//
// Do the pathfind
//


void cAILocoAction::StrokeDest(mxs_vector *pDest, tAIPathCellID *hintCell, const ObjID id)
{
// @TBD (toml 04-03-98): find cell needs some reworking for this to work as coded
   *hintCell = AIFindClosestCell(*pDest, *hintCell, id);

   if (*hintCell)
   {
      // @Note (toml 05-07-99): This logic in essense assumes all AIs are ground-based, and
      // should be redone if we introduce swimming/flying, etc
      const float kMaximumOffGround = 4.0;
      float zDestReachable = AIGetZAtXYOnCell(*pDest, *hintCell) + kMaximumOffGround;
      pDest->z = min(pDest->z, zDestReachable);
   }
}


// Set path. Does not AddRef.
void cAILocoAction::SetPath(IAIPath *pPath)
{
   if (!g_fAIPathFindInited)
      return;

   SafeRelease(m_pPath);

   m_pPath = pPath;

   if (!m_pPath->Active())
   {
      // it turns out that the path the does have any edges,
      // so we need to head directly to th final destination.
      flags |= kAIAF_HeadingDirect;
   }
}


STDMETHODIMP_(IAIPath *) cAILocoAction::GetPath()
{
   return m_pPath;
}


DECLARE_TIMER(AI_AILA_P, Average);

BOOL cAILocoAction::Pathfind(BOOL bTryNearEnough, float fNearDist)
{
   IAIPathfinder * pPathfinder = m_pAI->AccessPathfinder();

   if (!g_fAIPathFindInited || !pPathfinder)
      return FALSE;

   AUTO_TIMER(AI_AILA_P);

   SafeRelease(m_pPath);

   const cMxsVector & loc      = *m_pAIState->GetLocation();
   IAIPath *          pNewPath = NULL;

   StrokeDest(&m_dest, &m_hintCell, m_object);

   if (((pNewPath = pPathfinder->Pathfind(m_dest, m_hintCell, kAIOK_Walk | m_pAI->GetState()->GetStressBits())) != NULL) ||
       (bTryNearEnough && ((pNewPath = pPathfinder->PathfindNear(m_dest, fNearDist, m_hintCell, kAIOK_Walk | m_pAI->GetState()->GetStressBits())) != NULL)))
   {
      m_pPath = pNewPath;
      AIWatch2(Path, m_pAIState->GetID(),
              "Pathfound from cell %d to cell %d", m_pAIState->GetPathCell(), m_hintCell);

      // okay, the pathfind succeeded, but there might not actually be path
      // if the request turned out to start and end in the same cell...
      if (!m_pPath->Active())
      {
         // it turns out that the path the does have any edges,
         // so we need to head directly to th final destination.

         flags |= kAIAF_HeadingDirect;
      }
   }
   else
   {
      // @TBD (toml 04-03-98): m_pAI->RememberUnpathableLocation(m_curGoal.destination, kAIDefUnpathablePeriod);
      AIWatch2(Path, m_pAIState->GetID(),
              "Failed to pathfind from cell %d to cell %d", m_pAIState->GetPathCell(), m_hintCell);

#ifndef SHIP
      if (AIIsWatched(Flow, pOwner->AccessOuterAI()->GetObjID()) ||
          AIIsWatched(Path, pOwner->AccessOuterAI()->GetObjID()))
      {
         static cAITimer timer(kAIT_2Hz);
         if (timer.Expired())
         {
            const cMxsVector & loc = *m_pAIState->GetLocation();

            mprintf("AI(%3d) Error cannot pathfind:\n"
                    "   Points:\n"
                    "      (%g, %g, %g) --> \n"
                    "      (%g, %g, %g)\n"
                    "   Cells: [Self:%d] --> [Target:%d]\n"
                    "   Zones (N): [Self:%d] --> [Target:%d]\n"
                    "   Zones (N+L): [Self:%d] --> [Target:%d]\n"
                    "   Zones (H): [Self:%d] --> [Target:%d]\n"
                    "   Zones (H+L): [Self:%d] --> [Target:%d]\n"
                    "   Target object: %d\n"
                    "   Ability '%s'\n",
                    pOwner->AccessOuterAI()->GetObjID(),
                    loc.x, loc.y, loc.z,
                    m_dest.x, m_dest.y, m_dest.z,
                    m_pAIState->GetPathCell(), AIFindClosestCell(m_dest, m_hintCell, m_object),
                    AIGetCellZone(kAIZone_Normal, m_pAIState->GetPathCell()), AIGetCellZone(kAIZone_Normal, AIFindClosestCell(m_dest, m_hintCell, m_object)),
                    AIGetCellZone(kAIZone_NormalLVL, m_pAIState->GetPathCell()), AIGetCellZone(kAIZone_NormalLVL, AIFindClosestCell(m_dest, m_hintCell, m_object)),
                    AIGetCellZone(kAIZone_HighStrike, m_pAIState->GetPathCell()), AIGetCellZone(kAIZone_HighStrike, AIFindClosestCell(m_dest, m_hintCell, m_object)),
                    AIGetCellZone(kAIZone_HighStrikeLVL, m_pAIState->GetPathCell()), AIGetCellZone(kAIZone_HighStrikeLVL, AIFindClosestCell(m_dest, m_hintCell, m_object)),
                    m_object,
                    pOwner->GetName());
         }
      }
#endif
      return FALSE;
   }
   return TRUE;
}

///////////////////////////////////////
//
// Generate suggestions to continue on path
//

#define kDistDoAdvanceXY (2.3)
#define kDistDoAdvanceZ (6.0)

DECLARE_TIMER(AI_AILA_GS, Average);

#ifndef SHIP
BOOL g_bForcePathfind = FALSE;
#endif

HRESULT cAILocoAction::GenerateSuggestions(tAIPathOkBits nStressBits)
{
   AUTO_TIMER(AI_AILA_GS);

   if (!m_pPath)
   {
      Warning(("AI %d generating loco suggestions with no path\n", m_pAIState->GetID()));
      return E_FAIL;
   }

   const cAIPathEdge * pCurEdge;
   const cMxsVector & loc = *m_pAIState->GetLocation();

#ifndef SHIP
   if (g_bForcePathfind)
   {
      g_bForcePathfind = FALSE;
      Pathfind(FALSE, 0);
   }
#endif

   AssertMsg(m_pPath->Active() || (flags & kAIAF_HeadingDirect), "Loco action should have ended");

   // If the path is active, check for advancement
   if (m_pPath->Active())
   {
#ifndef SHIP
      const cAIPathEdge * pPrevCurEdge = m_pPath->GetCurEdge();
      int look =
#endif
      // Look ahead/back for clear line target. This will never look past the
      // final edge.
      m_pPath->UpdateTargetEdge(loc, ObjPosGet(m_pAIState->GetID())->fac, m_pAIState->GetPathCell(), nStressBits);

      // Now try to advance
      if ((pCurEdge = m_pPath->GetCurEdge()) != NULL)
      {
#ifndef SHIP
         AIWatchTrue9(pPrevCurEdge && look != 0, Path, m_pAIState->GetID(),
                      "Looked %s %d edges from [cell %d, edge [%d (%d,%d)]] to edge [%d (%d,%d)]",
                      (look > 0) ? "ahead" : "back",
                      (look > 0) ? look : -1 * look,
                      m_pAIState->GetPathCell(),
                      pPrevCurEdge->cell, pPrevCurEdge->vertexA, pPrevCurEdge->vertexB,
                      pCurEdge->cell, pCurEdge->vertexA, pCurEdge->vertexB);
#endif

         cMxsVector  edgeCenter;

         pCurEdge->GetCenter(&edgeCenter);

         // We advance to the next edge if Z and XY tolerences are met
         if (aflt(edgeCenter.z - loc.z, kDistDoAdvanceZ + m_pAI->GetGroundOffset() / 2) && // @tbd: getgroundoffset() is property lookup -- move to sAIState
             AIXYDistanceSq(edgeCenter, loc) < sq(kDistDoAdvanceXY))
         {
#ifndef SHIP
            pPrevCurEdge = m_pPath->GetCurEdge();
#endif
            HRESULT result = m_pPath->Advance(nStressBits);
            if (FAILED(result))
               return E_FAIL;

            if (result == S_FALSE)
            {
               // We reached the end of the path, so kill the path, and head towards the final destination.
               m_pPath->Clear();
               flags |= kAIAF_HeadingDirect;
               AIWatch(Path, m_pAIState->GetID(), "Reached end of path");
            }
#ifndef SHIP
            else // result == S_ADVANCE_REPATHFOUND || result == S_OK
            {
               pCurEdge = m_pPath->GetCurEdge();
               AIWatch6(Path, m_pAIState->GetID(), "Reached waypoint: advancing on path from edge [%d (%d,%d)] to edge [%d (%d,%d)]",
                        pPrevCurEdge->cell, pPrevCurEdge->vertexA, pPrevCurEdge->vertexB,
                        pCurEdge->cell, pCurEdge->vertexA, pCurEdge->vertexB);
            }
#endif
         }

      }
      else
      {
         // we can't get the current path edge, so kill the path, and head towards the final destination.
         m_pPath->Clear();
         flags |= kAIAF_HeadingDirect;
         AIWatch(Path, m_pAIState->GetID(), "Path is empty");
      }
   }

   // If we're not on the target location, make a suggestion towards it
   cMxsVector suggestLoc;

   if ((flags & kAIAF_HeadingDirect) ||
       (!m_pPath->MoreEdges() && m_pPath->IsDestVisible(loc, m_pAIState->GetPathCell(), m_pAIState->GetStressBits())))
   {
      // we're heading to final destination in the m_dest.  whoo hoo.
      suggestLoc = m_dest;
      AIWatch4(Path, m_pAIState->GetID(), "Suggesting move to target location (%d, %d, %d) [%g feet]", (int)m_dest.x, (int)m_dest.y, (int)m_dest.z, AIDistance(m_dest, loc));
   }
   else
   {
      // we go the current edge of the current path.
      // lets head towards the center of the edge.
      pCurEdge = m_pPath->GetCurEdge();
      pCurEdge->GetCenter(&suggestLoc);
      AIWatch4(Path, m_pAIState->GetID(), "Suggesting move to (%d, %d, %d) [%g feet]", (int)suggestLoc.x, (int)suggestLoc.y, (int)suggestLoc.z, AIDistance(suggestLoc, loc));
   }

   if (m_pMoveAction)
      m_pMoveAction->Clear();

   #define kLocoEpsXY 0.01
   #define kLocoEpsZ  0.50
   if (ffabsf(loc.x - suggestLoc.x) < kLocoEpsXY &&
       ffabsf(loc.y - suggestLoc.y) < kLocoEpsXY &&
       ffabsf(loc.z - suggestLoc.z) < kLocoEpsZ)
      return S_FALSE;

   if (!m_pMoveAction)
   {
      m_pMoveAction = CreateMoveAction();
      Assert_(m_pMoveAction);
   }

   sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

   pSuggestion->SetWeightedBias(m_moveSuggKind, 100);
   pSuggestion->dirArc.SetByCenterAndSpan(m_pAIState->AngleTo(suggestLoc),
                                          floatang(PI*1.8));
   pSuggestion->speed       = m_speed;
   pSuggestion->facing.type = kAIF_MoveDir;
   pSuggestion->dest        = suggestLoc;

   m_pMoveAction->Add(pSuggestion);
   m_pMoveAction->ClearTags();
   m_pMoveAction->AddTags(tags);

   return S_OK;
}

///////////////////////////////////////
//
// Set to an action that encompasses an entire "goto" goal
//

void cAILocoAction::SetFromGoal(const cAIGotoGoal * pGoal)
{
   if (!pGoal->location.IsValid())
      Set(pGoal->object, pGoal->speed, pGoal->accuracySq, pGoal->accuracyZ);
   else
      Set(pGoal->location, pGoal->object, pGoal->speed, pGoal->accuracySq, pGoal->accuracyZ);
}

///////////////////////////////////////

float cAILocoAction::ComputePathDist()
{
   if (!m_pPath && !Pathfind(!!(fPathNearDist > 0), fPathNearDist))
      return 0.0;
   return m_pPath->ComputeXYLength(*m_pAIState->GetLocation());
}

///////////////////////////////////////
//
// Update the action.
//

DECLARE_TIMER(AI_AILA_U, Average);

STDMETHODIMP_(eAIResult) cAILocoAction::Update()
{
   // If we haven't already resolved...
   if (InProgress())
   {
      AUTO_TIMER(AI_AILA_U);
      // If we're not doing anything, there's nothing to update
      if (!m_pMoveAction)
      {
         result = kAIR_NoResultSwitch;
      }
      // Otherwise, check progress...
      else
      {
         result = m_pMoveAction->Update();

         if (result == kAIR_NoResultSwitch)
         {
            eAIResult pathResult = UpdatePathProgress();

            if (pathResult > kAIR_NoResultSwitch)
               result = pathResult; // Locomotion complete, or error
         }
      }

      if (result > kAIR_NoResultSwitch)
      {
         // We're done, or there was an error
         eAIResult tempResult = result;
         ClearPathAndMove();
         result = tempResult;
         pctComplete = 100;
         AIWatch1(Path, m_pAIState->GetID(), "Path traversal %s", (result == kAIR_Success) ? "complete" : "failed");
      }
   }

   return result;
}

///////////////////////////////////////
//
// Enact the action.
//

DECLARE_TIMER(AI_AILA_E, Average);

STDMETHODIMP_(eAIResult) cAILocoAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(AI_AILA_E);

   if (!Started())
   {
      SetStarted(TRUE);
      if (!m_pPath && !Pathfind(!!(fPathNearDist > 0), fPathNearDist))
      {
         result = kAIR_Fail;
         return result;
      }
   }
   else if (!CanEnact())
      return result;


   // @TBD: timer. for when alert level goes back down, but we want to be able to jump
   // down for awhile.

   HRESULT suggestRes = GenerateSuggestions(m_pAI->GetState()->GetStressBits());

   if (suggestRes == E_FAIL)
      result = kAIR_Fail;
   else if (suggestRes == S_OK)
      result = m_pMoveAction->Enact(deltaTime);

   else
      result = kAIR_Success;

   return result;
}

///////////////////////////////////////////////////////////////////////////////
