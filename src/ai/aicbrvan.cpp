/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrvan.cpp,v 1.2 1999/03/29 08:47:34 JON Exp $
//
// AI Combat - ranged
//

#include <aicbrvan.h>

#include <appagg.h>
#include <comtools.h>

#include <contain.h>
#include <iobjsys.h>
#include <objdef.h>
#include <phprop.h>
#include <rendprop.h>

#include <aiactloc.h>
#include <aidebug.h>
#include <aiprrngd.h>
#include <aitrginf.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

int cAIRangedVantage::SuggestApplicability(void)
{
   // @TODO: get this working...
   // Check if our curent location has dropped significantly
   /*
   if ((GetLocationScore() == 0) ||
      (GetCurrentLocationScore() < (GetLocationScore() * RE_EVAL_CUR_LOC)))
      return kAIRC_AppNormal;
   */
   return 0;
}

////////////////////////////////////////

int cAIRangedVantage::SuggestInterrupt(void)
{
   // @TODO: Check for our dest going bad
   return kAIRC_AppNone;
}

////////////////////////////////////////

BOOL cAIRangedVantage::CheckPreconditions(void)
{
   return (m_pOwner->SelectMarkerMoveLoc(m_pLoc) != kAIRC_NoMove);
}

////////////////////////////////////////

cAIAction* cAIRangedVantage::SuggestAction(void)
{
   Assert_(m_pLoc != NULL);
   
   cAILocoAction * pLocoAction = m_pOwner->CreateLocoAction();
   pLocoAction->Set(*m_pLoc, kAIS_VeryFast);
   
   return (cAIAction*) pLocoAction;
}









