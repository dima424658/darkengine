/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaidmr.cpp,v 1.3 1999/04/01 13:53:44 JON Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>

#include <autolink.h>
#include <doorphys.h>
#include <linkbase.h>
#include <linkman.h>
#include <lnkquery.h>
#include <phlisbas.h>
#include <relation.h>

#include <aibascmp.h>
#include <aiapipth.h>

#include <shkaidmr.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static IRelation* g_pSwitchLinkRelation;
static IRelation* g_pTripwireRelation;

void ShockAIInitDoorMovReg(void)
{
   AutoAppIPtr(LinkManager);
   g_pSwitchLinkRelation = pLinkManager->GetRelationNamed("SwitchLink");
   Assert_(g_pSwitchLinkRelation);
   g_pTripwireRelation = pLinkManager->GetRelationNamed("Tripwire");
   Assert_(g_pTripwireRelation);
}

///////////////////////////////////////////////////////////////////////////////

void ShockAITermDoorMovReg(void)
{
   SafeRelease(g_pSwitchLinkRelation);
   SafeRelease(g_pTripwireRelation);
}

///////////////////////////////////////////////////////////////////////////////

cShockAIDoorMovReg::cShockAIDoorMovReg()
 : m_doorWait(AICustomTime(600)),
   m_waiting(FALSE),
   m_wantsInterrupt(FALSE)
{
}

///////////////////////////////////////

void cShockAIDoorMovReg::Init()
{
   SetNotifications(kAICN_Collision);
   SetNotifications(kAICN_Tripwire);
   m_doorWait.Force();
}

///////////////////////////////////////

void cShockAIDoorMovReg::End()
{
   ClearNotifications(kAICN_Collision);
   ClearNotifications(kAICN_Tripwire);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cShockAIDoorMovReg::GetName()
{
   return "Shock Door Regulator";
}

///////////////////////////////////////

void cShockAIDoorMovReg::OpenDoor(ObjID doorID)
{
   // Open the door if not already open, not opening & the pathfinder tells us we can
   if (GetDoorStatus(doorID) == kDoorOpen)
      Warning(("cShockAIDoorMovReg::OnCollision - %d collided with open door %d\n", GetID(), doorID));
   else if (GetDoorStatus(doorID) == kDoorClosed)
   {
      if (m_pAI->AccessPathfinder()->AccessControl()->CanPassDoor(doorID))
      {
         DoorPhysActivate(doorID, DOORPHYS_OPEN);
         /*
         // This is dangerous... if we missed the tripwire enter, we may miss the exit too
         // that means we may drag around a ton of links with us...
         AutoAppIPtr(LinkManager);
         pLinkManager->Add(doorID, GetID(), g_pTripwireRelation->GetID());
         */
      }
      else
         Warning(("cShockAIDoorMovReg::OnCollision - %d collided with unopenable door %d\n", GetID(), doorID));
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cShockAIDoorMovReg::OnCollision(const sPhysListenMsg* pMsg)
{
   if (IsDoor(pMsg->collObj))
      OpenDoor(pMsg->collObj);
}

///////////////////////////////////////

STDMETHODIMP_(void) cShockAIDoorMovReg::OnTripwire(ObjID objID, BOOL enter)
{
   // assume we are a door here, not really right I guess
   if (GetDoorStatus(objID) != kDoorOpen)
   {
      m_wantsInterrupt = TRUE;
      m_doorID = objID;
   }
}

///////////////////////////////////////

#define kDoorOpenAngle     DEGREES(60.0)
#define kDoorOpenDistSq    sq(2.)

STDMETHODIMP_(BOOL) cShockAIDoorMovReg::AssistGoal(const sAIMoveGoal & goal, sAIMoveGoal * pResultGoal)
{
   if (m_waiting)
   {
      // check our angle to door
      cMxsVector doorVec;
      floatang doorAng;
      float doorDistSq;

      GetObjLocation(m_doorID, &doorVec);
      doorAng = Delta(m_pAIState->AngleTo(doorVec), goal.dir);
      doorDistSq = AIXYDistanceSq(doorVec, goal.dest);
      if ((doorAng < kDoorOpenAngle) && (doorDistSq<kDoorOpenDistSq))
         m_doorWait.Reset();
      m_waiting = FALSE;
   }
   if (goal.speed && !m_doorWait.Expired())
   {
      pResultGoal->dir   = m_pAIState->GetFacingAng();
      pResultGoal->dest  = *m_pAIState->GetLocation();
      pResultGoal->speed = kAIS_Stopped;
      return TRUE;
   }
   return FALSE; // didn't change the goal
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cShockAIDoorMovReg::WantsInterrupt()
{
   if (m_wantsInterrupt)
   {
      m_waiting = TRUE;
      m_wantsInterrupt = FALSE;
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
