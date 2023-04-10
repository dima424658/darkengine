/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrmod.cpp,v 1.2 1999/03/29 08:45:04 JON Exp $
//
// AI Combat - ranged
//

#include <aicbrmod.h>

#include <appagg.h>
#include <comtools.h>
#include <ctagset.h>
#include <objpos.h>
#include <wrtype.h>

#include <aiactmot.h>
#include <aibasact.h>
#include <aicbrmpr.h>
#include <aidebug.h>
#include <aiprops.h>
#include <aiprrngd.h>
#include <aitrginf.h>
#include <aitype.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

EXTERN int g_AIRangedCombatModeWeights[kAIRC_NumApplicabilities] = 
{
   0,    // kAIRC_AppNone
   1,    // kAIRC_AppMinimum
   10,   // kAIRC_AppVeryLow
   50,   // kAIRC_AppLow
   100,  // kAIRC_AppNormal
   200,  // kAIRC_AppHigh
   1000, // kAIRC_AppVeryHigh
};

cAIAction* cAIRangedMode::CreateMotionAction(char* pTagString)
{
   cTagSet tags(pTagString);
   cAIAction* pAction = m_pOwner->CreateMotionAction();

   pAction->AddTags(tags);
   // all these want to face my opponent
   floatang direction = GetState()->AngleTo(GetTargetInfo()->loc);
   ((cAIMotionAction *)pAction)->SetFacing(direction);
   ((cAIMotionAction *)pAction)->SetFocus(GetTargetInfo()->id);
   return pAction;
}

int cAIRangedMode::GetApplicabilityWeight(int adj) const
{
   int app = AIGetRangedApplicabilities(GetID())->m_modeApplicability[GetModeID()]+adj;
   if (app<0)
      return g_AIRangedCombatModeWeights[0];
   else if (app>=kAIRC_NumApplicabilities)
      return g_AIRangedCombatModeWeights[kAIRC_NumApplicabilities-1];
   return g_AIRangedCombatModeWeights[app];
}

