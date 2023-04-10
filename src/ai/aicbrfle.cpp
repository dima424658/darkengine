/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrfle.cpp,v 1.1 1999/03/29 08:43:10 JON Exp $
//
// AI Combat - ranged flee
//

#include <aicbrfle.h>

#include <appagg.h>
#include <comtools.h>

#include <contain.h>
#include <iobjsys.h>
#include <objdef.h>
#include <phprop.h>
#include <rendprop.h>

#include <aiactloc.h>
#include <aicbrmpr.h>
#include <aidebug.h>
#include <aipathdb.h>
#include <aiprops.h>
#include <aiprrngd.h>
#include <aitrginf.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

int cAIRangedFlee::SuggestApplicability(void)
{
   if (GetRangeClass()>kAIRC_RangeShort)
      return 0;
   return g_AIRangedCombatModeWeights[AIGetRangedFlee(GetID())->m_rangeApplicability[GetRangeClass()]];
}

////////////////////////////////////////

int cAIRangedFlee::SuggestInterrupt(void)
{
   // @TODO: Remove this?
   return kAIRC_AppNone;
}

////////////////////////////////////////

BOOL cAIRangedFlee::CheckPreconditions(void)
{
   mxs_vector targetVec;
   Location targetLoc;
   const Location* pMyLoc = &GetCurrentLocation()->GetLocation();
   BOOL found = FALSE;
   floatang angle;
   floatang bestAngle;
   const sAIRangedFleeParams* pParams = AIGetRangedFlee(GetID());
   cAIRangedLocation* pTestAILoc = new cAIRangedLocation(m_pOwner);

   for (int i=0; i<pParams->m_numPoints; i++)
   {
      // calc angle
      angle = (float(i)*(2.*pParams->m_angleRange)/float(pParams->m_numPoints-1))-pParams->m_angleRange;
      angle = angle+PI-GetTargetHeading();
      floatang_normalize_angle(angle.value);
      
      // find point
      ProjectFromLocationOnZPlane(pMyLoc->vec, pParams->m_distance+pParams->m_clearance, angle, &targetVec);
      MakeHintedLocationFromVector(&targetLoc, &targetVec, pMyLoc);
      pTestAILoc->SetLocation(targetLoc);

      if (pTestAILoc->TestPathcast())
      {
         pTestAILoc->TestLOF();
         pTestAILoc->TestRangeClass();
         pTestAILoc->Score();
         if (((m_pTargetAILoc == NULL) && (pTestAILoc->GetScore()>GetCurrentLocation()->GetScore())) 
            || ((m_pTargetAILoc != NULL) && (pTestAILoc->GetScore()>m_pTargetAILoc->GetScore())))
         {
            if (m_pTargetAILoc != NULL)
               delete m_pTargetAILoc;
            m_pTargetAILoc = pTestAILoc;
            pTestAILoc = new cAIRangedLocation(m_pOwner);
            bestAngle = angle;
            found = TRUE;
         }
      }
   }
   if (found)
   {
      ProjectFromLocationOnZPlane(pMyLoc->vec, pParams->m_distance, bestAngle, &targetVec);
      MakeHintedLocationFromVector(&targetLoc, &targetVec, pMyLoc);
      m_pTargetAILoc->SetLocation(targetLoc);
      m_pTargetAILoc->TestPathcast();  // to get cell ID
      targetLoc.vec.z = g_AIPathDB.GetZAtXY(m_pTargetAILoc->GetCellID(), targetVec) + 3.0;
      m_pTargetAILoc->SetLocation(targetLoc);
   }
   return found;   
}

////////////////////////////////////////

cAIAction* cAIRangedFlee::SuggestAction(void)
{
   Assert_(m_pTargetAILoc != NULL);
   
   cAILocoAction * pLocoAction = m_pOwner->CreateLocoAction();
   pLocoAction->Set(m_pTargetAILoc->GetLocation().vec, kAIS_VeryFast);
   delete m_pTargetAILoc;
   m_pTargetAILoc = NULL;
   return (cAIAction*) pLocoAction;
}









