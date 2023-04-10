/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ainoncbt.cpp,v 1.13 2000/02/19 12:44:50 toml Exp $
//
//
//

#include <appagg.h>
#include <playrobj.h>
#include <relation.h>

#include <aialert.h>
#include <aiaware.h>
#include <ainoncbt.h>
#include <aiapinet.h>
#include <aiapisnd.h>
#include <aisndtyp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINonCombatAbility
//

cAINonCombatAbility::cAINonCombatAbility()
 : m_BroadcastTimer(kAIT_2Sec)
{
   SetNotifications(kAICN_BeginFrame | kAICN_Alertness);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINonCombatAbility::OnBeginFrame()
{
   if (m_pAIState->GetMode() < kAIM_Normal)
      return;

   if (InControl())
   {
      if (m_BroadcastTimer.Expired())
      {
         m_BroadcastTimer.Reset();
         BroadcastAlertness();
      }
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINonCombatAbility::OnAlertness(ObjID source, eAIAwareLevel previous,  eAIAwareLevel current, const sAIAlertness * pRaw)
{
   cAIAbility::OnAlertness(source, previous,  current, pRaw);

   if (!InControl() || !m_pAI->AccessSoundEnactor())
      return;

   eAISoundConceptEnum concept = kAISC_CoreNum;
   
   switch (current)
   {
      case kAIAL_Lowest:
         concept = kAISC_AlertDownToZero;
         break;

      case kAIAL_Low:
         if (previous < kAIAL_Low)
            concept = kAISC_AlertToOne;
         break;

      case kAIAL_Moderate:
         if (previous < kAIAL_Moderate)
            concept = kAISC_AlertToTwo;
         break;

      case kAIAL_High:
      {
         const sAIAwareness * pAwareness;;

         AutoAppIPtr(AINetServices);
         tAIPlayerList players;
         
         pAINetServices->GetPlayers(&players);
         
         ObjID * pPlayerID = &players[0];

         concept = kAISC_AlertToThree;
         
         while (*pPlayerID)
         {
            pAwareness = m_pAI->GetAwareness(*pPlayerID);
            if (pAwareness->flags & kAIAF_HaveLOS)
            {
               concept = kAISC_SpotPlayer;
               break;
            }
            pPlayerID++;
         }
         
         break;
      }
   }
   
   if (concept != kAISC_CoreNum)
      m_pAI->AccessSoundEnactor()->RequestConcept(concept);
}

///////////////////////////////////////

void cAINonCombatAbility::BroadcastAlertness()
{
   static eAISoundConcept alertToConcept[] = 
   {
      kAISC_AlertZeroBroadcast,
      kAISC_AlertOneBroadcast,
      kAISC_AlertTwoBroadcast,
      kAISC_AlertThreeBroadcast,
      kAISC_CoreNum,
   };

   eAISoundConcept concept = alertToConcept[m_pAIState->GetAlertness()];
   
   // mprintf("My alertness is %s, requesting %s\n", AIGetAlertLevelName(m_pAIState->GetAlertness()), AIGetConceptName(concept));
   
   if (m_pAI->AccessSoundEnactor() && concept != kAISC_CoreNum)
   {
      m_pAI->AccessSoundEnactor()->RequestConcept(concept);
   }
}

///////////////////////////////////////////////////////////////////////////////

