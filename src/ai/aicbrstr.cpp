/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrstr.cpp,v 1.2 1999/03/29 08:47:22 JON Exp $
//
// AI Combat - ranged StepRight
//

#include <aicbrstr.h>

#include <appagg.h>
#include <comtools.h>

#include <playrobj.h>

#include <objpos.h>

#include <aiactloc.h>
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

// I hate this
const float kAIRangedStepRightDist = 5.;

////////////////////////////////////////

int cAIRangedStepRight::SuggestApplicability(void)
{
   if (!m_pOwner->GetCurrentLocation()->GetLOF())
      return GetApplicabilityWeight(0);
   return 0;
}

////////////////////////////////////////

BOOL cAIRangedStepRight::CheckPreconditions(void)
{
   float backang=GetState()->GetFacingAng().value;
   mxs_vector destVec;
   Location destLoc;
   cAIRangedLocation AIDestLoc(m_pOwner);

   // @TODO: somehow, these numbers need to be derived from the motion
   // or at least, a property
   destVec.x=kAIRangedStepRightDist*sin(backang);  
   destVec.y=-kAIRangedStepRightDist*cos(backang);  
   destVec.z=0;
   mx_addeq_vec(&destVec, GetState()->GetLocation());
   MakeHintedLocationFromVector(&destLoc, &destVec, GetState()->GetPortalLocation());
   AIDestLoc.SetLocation(destLoc);
   if (!AIDestLoc.TestPathcast())
      return FALSE;
   return AIDestLoc.TestLOF();
   return FALSE;
}
  
////////////////////////////////////////
 
cAIAction* cAIRangedStepRight::SuggestAction(void)
{
   return CreateMotionAction("Locomote 0, Direction 2");
}









