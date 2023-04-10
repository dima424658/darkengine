/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaidth.cpp,v 1.2 1999/11/19 14:54:45 adurant Exp $
//
//
//

This file has been moved to aiqdeath.cpp AMSD



#include <shkaidth.h>

#include <lg.h>
#include <appagg.h>

#include <aiapisnd.h>
#include <aidebug.h>
#include <aisndtyp.h>

#include <creatext.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIQuickDeath
//

STDMETHODIMP_(const char *) cAIQuickDeath::GetName()
{
   return "QuickDeath";
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIQuickDeath::Init()
{
   // Intentially not calling base -- want no goals or notifications by default
   SetNotifications(kAICN_Death);
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIQuickDeath::OnDeath(const sDamageMsg * pMsg)
{
   // Halt all current sounds
   if (m_pAI->AccessSoundEnactor())
      m_pAI->AccessSoundEnactor()->HaltCurrent();

   CreatureBeDead(m_pAI->GetObjID());
   m_pAI->Kill();

   AIWatch(Death, m_pAI->GetObjID(), "quick slain");
}



