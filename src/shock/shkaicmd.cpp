/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaicmd.cpp,v 1.1 1999/06/18 22:07:20 JON Exp $
//
//
//

#include <shkaicmd.h>

#include <lg.h>
#include <appagg.h>

#include <aiapisnd.h>
#include <aidebug.h>
#include <aigoal.h>
#include <aisndtyp.h>

#include <creatext.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICameraDeath
//

STDMETHODIMP_(const char *) cAICameraDeath::GetName()
{
   return "CameraDeath";
}

//////////////////////////////////////

STDMETHODIMP_(void) cAICameraDeath::Init()
{ 
   // Intentially not calling base -- want no goals or notifications by default
   SetNotifications(kAICN_Death);

   m_dying = FALSE;
}

//////////////////////////////////////

STDMETHODIMP_(void) cAICameraDeath::OnDeath(const sDamageMsg * pMsg)
{
   // Halt all current sounds
   if (m_pAI->AccessSoundEnactor())
      m_pAI->AccessSoundEnactor()->HaltCurrent();

   m_dying = TRUE;
   SignalGoal();

   AIWatch(Death, m_pAI->GetObjID(), "camera death");
}

//////////////////////////////////////

STDMETHODIMP cAICameraDeath::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (!m_dying)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }
   
   *ppGoal = new cAIDieGoal(this);
   (*ppGoal)->priority= kAIP_VeryHigh;

   SignalAction();

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAICameraDeath::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   m_pAI->Kill();

   return S_OK;
}
