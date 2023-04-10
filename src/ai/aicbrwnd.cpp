/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrwnd.cpp,v 1.2 1999/03/29 08:47:47 JON Exp $
//
// AI Combat - ranged wound
//

#include <aicbrwnd.h>

#include <appagg.h>
#include <comtools.h>

#include <playrobj.h>

#include <objpos.h>

#include <aiactloc.h>
#include <aiapisns.h>
#include <aiapisnd.h>
#include <aidebug.h>
#include <aiprrngd.h>
#include <aiprops.h>
#include <aipthfnd.h>
#include <aisndtyp.h>
#include <aitrginf.h>
#include <aitype.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

int cAIRangedWound::SuggestInterrupt(void)
{
   sAIRangedCombatEvent* pEvent = m_pOwner->GetEvent(kAIRC_EventWounded);

   if (pEvent)
   {
      sAIRangedWound* pWoundResponse = AIGetRangedWound(GetID());
      // drop out if damage below threshold
      if (int(pEvent->m_pData)<pWoundResponse->m_woundThreshold)
         return 0;
      // drop out randomly
      if (AIRandom(0, 100)>pWoundResponse->m_responseChance)
         return 0;
      return GetApplicabilityWeight(0);
   }
   else 
      return 0;
}

////////////////////////////////////////

int cAIRangedWound::SuggestApplicability(void)
{
   return SuggestInterrupt();
}

////////////////////////////////////////
 
cAIAction* cAIRangedWound::SuggestAction(void)
{
   sAIRangedCombatEvent* pEvent = m_pOwner->GetEvent(kAIRC_EventWounded);

   Assert_(pEvent); 
   if (int(pEvent->m_pData)>AIGetRangedWound(GetID())->m_severeThreshold)
      return CreateMotionAction("ReceiveWound 0, SevereWound 0");
   else
      return CreateMotionAction("ReceiveWound 0");
}









