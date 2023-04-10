/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaicbr.cpp,v 1.2 1999/05/29 21:43:54 JON Exp $
//
// Shock ranged combat
//
 
#include <shkaicbr.h>

#include <shkaicrb.h>

#include <aiapi.h>
#include <aibascmp.h>
#include <aicbrclo.h>
#include <aicbrfle.h>
#include <aicbridl.h>
#include <aicbrsht.h>
#include <aicbrstl.h>
#include <aicbrstr.h>
#include <aicbrloc.h>
#include <aicbrwnd.h>
#include <aimultcb.h>
#include <aiprcore.h>
#include <aiprrngd.h>
#include <aiteams.h>

#include <autolink.h>
#include <doorphys.h>
#include <linkman.h>
#include <relation.h>

#include <physapi.h>
#include <phmodobb.h>

#include <objhp.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////
 
STDMETHODIMP_(void) cAIShockRangedSubcombat::Init()
{
   cAISubcombat::Init();

   SetNotifications(kAICN_BeginFrame |
                    kAICN_ActionProgress |
                    kAICN_Damage |
                    kAICN_GoalChange |
                    kAICN_ModeChange);

   // cache prop
   sAIRangedCombatProp *pRangedProp = AIGetRangedCombat(GetID());

   // location
   m_pCurrentLocation = new cAIRangedLocation(this, *m_pAIState->GetPortalLocation());

   // create modes, if we were cool, these would be installed like components
   // @TODO: dont add modes if applicability is too low   
   m_numberModes = 0;
   m_pModes[m_numberModes++] = new cAIRangedIdle(this);
   m_pModes[m_numberModes++] = new cAIRangedShoot(this);
   m_pModes[m_numberModes++] = new cAIRangedClose(this);
   m_pModes[m_numberModes++] = new cAIShockRangedBackup(this);
   m_pModes[m_numberModes++] = new cAIRangedWound(this);
   m_pModes[m_numberModes++] = new cAIRangedStepRight(this);
   m_pModes[m_numberModes++] = new cAIRangedStepLeft(this);
   m_pModes[m_numberModes++] = new cAIRangedFlee(this);

   // properties
   m_pRangedCombatProp = AIGetRangedCombat(GetID());
   m_pRanges = AIGetRangedRanges(GetID());
 
   // ranges
   // @TODO: prop listener or something
   m_rangesSq.m_ranges[0] = m_pRanges->m_ranges[0]*m_pRanges->m_ranges[0];
   m_rangesSq.m_ranges[1] = m_pRanges->m_ranges[1]*m_pRanges->m_ranges[1];
   m_rangesSq.m_ranges[2] = m_pRanges->m_ranges[2]*m_pRanges->m_ranges[2];
   m_rangesSq.m_ranges[3] = m_pRanges->m_ranges[3]*m_pRanges->m_ranges[3];
 
   Reset();
}

///////////////////////////////////////////////////////
//
// Shock physcast callback for ranged combat ability
//

BOOL ShockRangedPhyscastCallback(ObjID objID, const cPhysModel* pModel, sAIRangedCombatPhyscastData* pData)
{
   // ignore myself & my target
   if ((objID == (pData)->sourceID) || (objID == (pData)->targetID))
      return FALSE;

   // ignore edge triggers
   if ((pModel->GetType(0) == kPMT_OBB) && ((cPhysOBBModel *)pModel)->IsEdgeTrigger())
      return FALSE;

   // ignore destroyable objects
   int hp;
   if (ObjGetHitPoints(objID, &hp))
   {
      // except, don't ignore other AIs that are team-mates
      AutoAppIPtr(AIManager);
      IAI* pAI;
      if ((pAI = pAIManager->GetAI(objID)) != NULL)
      {
         BOOL friendlyFire = (AITeamCompare(AIGetTeam(pData->sourceID), AIGetTeam(objID)) == kAI_Teammates);
         pAI->Release();
         return friendlyFire;
      }
      return FALSE;
   }
   return TRUE;
}

