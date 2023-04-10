/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrbkp.cpp,v 1.3 1999/04/11 10:21:45 JON Exp $
//
// AI Combat - ranged backup
//

#include <aicbrbkp.h>

#include <appagg.h>
#include <comtools.h>

#include <playrobj.h>

#include <objpos.h>

#include <aiactmot.h>
#include <aiapisns.h>
#include <aiapisnd.h>
#include <aidebug.h>
#include <aiprrngd.h>
#include <aipthfnd.h>
#include <aisndtyp.h>
#include <aitrginf.h>
#include <aitype.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

const floatang kAIRangedBackupHeadingDelta = PI/2;

int cAIRangedBackup::SuggestApplicability(void)
{
   if (GetRangeClass() <= kAIRC_RangeShort)
   {
      if (GetTargetHeadingDelta()<kAIRangedBackupHeadingDelta)
      {
         if (GetRangeClass() == kAIRC_RangeVeryShort)
            return GetApplicabilityWeight(0);
         else // (GetRangeClass() == kAIRC_RangeShort)
            return GetApplicabilityWeight(-1);
      }
   }
   return kAIRC_AppNone;
}

////////////////////////////////////////

const float kAIRangedBackupDist = 5.;

BOOL cAIRangedBackup::CheckPreconditions(void)
{
   cMxsVector destVec;
   Location destLoc;

   float backang=GetState()->GetFacingAng().value;
   // @TODO: somehow, these numbers need to be derived from the motion
   // or at least, a property
   destVec.x=-kAIRangedBackupDist*cos(backang);  
   destVec.y=-kAIRangedBackupDist*sin(backang);  
   destVec.z=0;
   mx_addeq_vec(&destVec, GetState()->GetLocation());
   MakeHintedLocationFromVector(&destLoc, &destVec, GetState()->GetPortalLocation());
   m_destAILoc.SetLocation(destLoc);
   if (!m_destAILoc.TestPathcast())
      return FALSE;
   m_destAILoc.TestLOF();
   m_destAILoc.TestRangeClass();
   m_destAILoc.Score();
   return (m_destAILoc.GetScore()>GetCurrentLocation()->GetScore());
} 
 
////////////////////////////////////////
 
cAIAction* cAIRangedBackup::SuggestAction(void)
{
   cAIMotionAction* pAction = (cAIMotionAction*)CreateMotionAction("Locomote 0, Direction 4");
   pAction->SetFocus(m_pOwner->GetTarget());
   return pAction;
}









