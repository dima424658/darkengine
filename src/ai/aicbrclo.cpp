/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrclo.cpp,v 1.7 2000/02/18 14:47:31 bfarquha Exp $
//
// AI Combat - ranged
//

#include <aicbrclo.h>

#include <appagg.h>
#include <comtools.h>

#include <playrobj.h>

#include <objpos.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiapisns.h>
#include <aiapisnd.h>
#include <aidebug.h>
#include <aiprrngd.h>
#include <aisndtyp.h>
#include <aitrginf.h>
#include <aitype.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

const tSimTime kAIRC_CloseRePathTime = 3000;
const float kAIRC_CloseRePathDistSq = 16.;

////////////////////////////////////////

cAIRangedClose::cAIRangedClose(cAINewRangedSubcombat* pOwner):
   cAIRangedMode(pOwner),
   m_pLocoAction(NULL),
   m_rePathBlock(FALSE)
{
   SetFlags(kAIRC_MoveMode);
}

////////////////////////////////////////

void cAIRangedClose::OnLosingControl(void)
{
}

////////////////////////////////////////

int cAIRangedClose::SuggestApplicability(void)
{
   // @TODO: check on facing
   // @TODO: really evaluate final location
   if (GetRangeClass() == kAIRC_RangeVeryLong)
      return GetApplicabilityWeight(0);
   if (GetRangeClass() == kAIRC_RangeLong)
      return GetApplicabilityWeight(-1);
   // this is kind of a hack.

   // wsf: The RangeVeryShort part of the condition was commented out. Why?
   //      With this only, AI's, when very close (because LOF fails), they will just run strait at you.
   //      When close to player, they should only do this if nothing else is appropriate (like when they're trapped).
   //      This applicability stuff is strange. The

   // if (!GetCurrentLocation()->GetLOF()) // (GetRangeClass() != kAIRC_RangeVeryShort))
   //   return GetApplicabilityWeight(kAIRC_AppMinimum);

   if (!GetCurrentLocation()->GetLOF())
   {
      if (GetRangeClass() > kAIRC_RangeShort)
         return GetApplicabilityWeight(kAIRC_AppMinimum);
// else what? If we're close to target, then don't run at him. That's dumb.
//      else
//         return GetApplicabilityWeight(kAIRC_AppMinimum);
   }

   return 0;
}

////////////////////////////////////////

const float kAIRangedForwardDist = 5.;

BOOL cAIRangedClose::CheckStepForwardPreconditions(void)
{
   cMxsVector destVec;
   Location destLoc;
   cAIRangedLocation destAILoc(m_pOwner);

   float facing = GetState()->GetFacingAng().value;
   // @TODO: somehow, these numbers need to be derived from the motion
   // or at least, a property
   destVec.x=kAIRangedForwardDist*cos(facing);
   destVec.y=kAIRangedForwardDist*sin(facing);
   destVec.z=0;
   mx_addeq_vec(&destVec, GetState()->GetLocation());
   MakeHintedLocationFromVector(&destLoc, &destVec, GetState()->GetPortalLocation());
   destAILoc.SetLocation(destLoc);
   if (!destAILoc.TestPathcast())
      return FALSE;
   destAILoc.TestLOF();
   destAILoc.TestRangeClass();
   destAILoc.Score();
   return (destAILoc.GetScore()>=GetCurrentLocation()->GetScore());
}

////////////////////////////////////////

BOOL cAIRangedClose::CheckLocoPreconditions(void)
{
   cMxsVector targetVec;

   Assert_(!m_pLocoAction);
   m_pLocoAction = m_pOwner->CreateLocoAction();
   targetVec = GetTargetInfo()->loc;
   targetVec.z = GetTargetInfo()->zLocNearFloor;
   m_pLocoAction->Set(targetVec, GetTarget(), kAIS_Fast, sq(5.0));
   if (!m_pLocoAction->Pathfind()) // @TODO: change loco action to save this path
   {
      SafeRelease(m_pLocoAction);
      m_pLocoAction = NULL;
      m_failedPathTime = GetSimTime();
      m_rePathBlock = TRUE;
      return FALSE;
   }
   return TRUE;
}

////////////////////////////////////////

BOOL cAIRangedClose::CheckPreconditions(void)
{
   if (m_rePathBlock)
      m_rePathBlock = (((GetSimTime()-m_failedPathTime)<kAIRC_CloseRePathTime) &&
         (AIDistanceSq(GetCurrentLocation()->GetLocation().vec, m_failedPathLoc.vec)<kAIRC_CloseRePathDistSq));
   if (!m_rePathBlock && CheckLocoPreconditions())
      return TRUE;
   return CheckStepForwardPreconditions();
}

////////////////////////////////////////

cAIAction* cAIRangedClose::SuggestStepForwardAction(void)
{
   cAIMotionAction* pAction = (cAIMotionAction*)CreateMotionAction("Locomote 0");
   pAction->SetFocus(m_pOwner->GetTarget());
   return pAction;
}

////////////////////////////////////////

static float g_RangedNoFreshenRange = 34.0;   // @TODO: do something sensible
static float g_RangedSpeed = 11.0;

cAIAction* cAIRangedClose::SuggestLocoAction(void)
{
   cAILocoAction* pLocoAction = m_pLocoAction;
   m_pLocoAction = NULL;

   IAISenses * pSenses = GetInternalAI()->AccessSenses();
   BOOL freshen = (pSenses && IsAPlayer(GetTargetInfo()->id));

   if (freshen)
   {
      float distKeepFreshSense = pLocoAction->ComputePathDist()  - g_RangedNoFreshenRange;

      if (distKeepFreshSense > 0.0)
         pSenses->KeepFresh(GetTargetInfo()->id, (distKeepFreshSense / g_RangedSpeed) * 1000.0);
   }

   if (GetInternalAI()->AccessSoundEnactor() && m_pOwner->CanPlayReactCharge())
   {
      GetInternalAI()->AccessSoundEnactor()->RequestConcept(kAISC_ReactCharge);
      m_pOwner->SetPlayedReactCharge();
   }
   return pLocoAction;
}

cAIAction* cAIRangedClose::SuggestAction(void)
{
   if (m_pLocoAction)
      return SuggestLocoAction();
   return SuggestStepForwardAction();
}









