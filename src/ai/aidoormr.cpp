/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidoormr.cpp,v 1.15 2000/03/02 09:50:01 toml Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>

#include <aidoormr.h>

#include <lockprop.h> //for proper door locking/unlocking.

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static IRelation * g_pAIDoorLinks; //this should be static. AMSD

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitDoorMovReg(IAIManager *)
{
   sRelationDesc     doorDesc   = { "AIDoor",        kRelationNetworkLocalOnly, 1, 0 };
   sRelationDataDesc doorDataDesc = LINK_DATA_DESC(int);

   g_pAIDoorLinks = CreateStandardRelation(&doorDesc, &doorDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));

   return TRUE;
}

///////////////////////////////////////

BOOL AITermDoorMovReg()
{
   SafeRelease(g_pAIDoorLinks);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

#define SLOW_LOOK 300
#define FAST_LOOK 200

cAIDoorMovReg::cAIDoorMovReg()
 : m_DoorLookPeriod(AICustomTime(SLOW_LOOK)),
   m_DoorWait(AICustomTime(600)),
   m_fWaiting(FALSE)
{

}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIDoorMovReg::GetName()
{
   return "Door regulator";
}

///////////////////////////////////////

#define kDoorDistSq        sq(6.0)
#define kDoorOpenAngle     DEGREES(35.0)
#define kDoorCloseAngle    DEGREES(60.0)
#define kDoorOpenDistSq    sq(4.5)
#define kDoorCloseDistSq   sq(4.0)

DECLARE_TIMER(AI_DMR, Average);

// @TBD: kAIS_VeryFast uses faster LookPeriod Timer.

STDMETHODIMP_(BOOL) cAIDoorMovReg::AssistGoal(const sAIMoveGoal & goal, sAIMoveGoal * pResultGoal)
{
   if (goal.speed && m_DoorLookPeriod.Expired())
   {
      AUTO_TIMER(AI_DMR);

      if (goal.speed >= kAIS_VeryFast)
         m_DoorLookPeriod.Set(AICustomTime(FAST_LOOK));
      else
         m_DoorLookPeriod.Set(AICustomTime(SLOW_LOOK));

      m_DoorLookPeriod.Reset();

      cMxsVector doorLoc;
      floatang   doorAng;
      float      distSq;
      LinkID     linkId = g_pAIDoorLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

      if (linkId)
      {
         sLink link;
         g_pAIDoorLinks->Get(linkId, &link);

         //if (AIDoorIsOpen(link.dest))
         //   m_DoorWait.Force();

         if (!m_DoorWait.Expired())
         {
            pResultGoal->dir = m_pAIState->GetFacingAng();
            pResultGoal->dest = *m_pAIState->GetLocation();
            pResultGoal->speed = kAIS_Stopped;
            return TRUE;
         }

         if (GetObjLocation(link.dest, &doorLoc))
         {
            doorAng = Delta(m_pAIState->AngleTo(doorLoc), goal.dir);
            if ((distSq = m_pAIState->DistSq(doorLoc)) > kDoorCloseDistSq &&
                doorAng > kDoorCloseAngle)
            {
               int doorlinkflags = *((int *)g_pAIDoorLinks->GetData(linkId));
               BOOL fWasAIOpened = doorlinkflags & kWasAIOpened;
               BOOL fWasAIUnlocked = doorlinkflags & kWasAIUnlocked; //so AI's don't relock doors they 
                                                                     //didn't unlock.
               //potential problem... AI may "forget" to relock door if another AI right behind him.
               //but people sometimes do that too.  "I thought you locked it!"  So this shouldn't
               //be a problem.  AMSD
               //BOOL fWasAIOpened = *((BOOL *)g_pAIDoorLinks->GetData(linkId));
               g_pAIDoorLinks->Remove(linkId);

               AutoIPtr(LinkQuery);
               pLinkQuery = g_pAIDoorLinks->Query(LINKOBJ_WILDCARD, link.dest);
               if (pLinkQuery->Done() && fWasAIOpened)
               {
                  AICloseDoor(GetID(), link.dest, fWasAIUnlocked);
               }
            }
            else if (distSq < kDoorOpenDistSq && doorAng < kDoorOpenAngle)
            {
               AIOpenDoor(GetID(), link.dest);
            }
         }
         else
         {
            // We seem to be linked to a door that has swung out of the world...
            g_pAIDoorLinks->Remove(linkId);
#ifndef SHIP
            mprintf("ERROR: Bad door: when AI %d opened door %d it exited the world!\n", GetID(), link.dest);
#endif
         }
      }
      else
      {
         const cDynArray<sAIDoorInfo> & doors = AIGetDoors();
         for (int i = 0; i < doors.Size(); i++)
         {
            if (GetObjLocation(doors[i].obj, &doorLoc) &&
                (distSq = m_pAIState->DistSq(doorLoc)) < kDoorDistSq)
            {
               doorAng = Delta(m_pAIState->AngleTo(doorLoc), goal.dir);
               if (distSq < kDoorOpenDistSq && doorAng < kDoorOpenAngle)
               {
                  BOOL fIsOpen      = AIDoorIsOpen(doors[i].obj);
                  BOOL fWasAIOpened = FALSE;
                  BOOL fWasAIUnlocked = FALSE;

                  AutoIPtr(LinkQuery);
                  pLinkQuery = g_pAIDoorLinks->Query(LINKOBJ_WILDCARD, doors[i].obj);
                  if (!pLinkQuery->Done())
                  {
                      int doorflags = *((int *)pLinkQuery->Data());
                      fWasAIOpened = doorflags & kWasAIOpened;
                      fWasAIUnlocked = doorflags & kWasAIUnlocked;
//                     fWasAIOpened = *((BOOL *)pLinkQuery->Data());
                  }
                  SafeRelease(pLinkQuery);

                  if (!fIsOpen && AICanOpenDoor(GetID(), doors[i].obj))
                  {
                     //BOOL trueVal = TRUE; //unused, was sent into AddFull below.
                     int doorflags = 0;
                     doorflags |= kWasAIOpened;
                     if (ObjComputeLockedState(doors[i].obj)) //is it locked?
                         doorflags |= kWasAIUnlocked;  //if it we had to unlock it, we unlocked it.
                     g_pAIDoorLinks->AddFull(GetID(), doors[i].obj, &doorflags);

                     m_fWaiting = TRUE;
                     AIOpenDoor(GetID(), doors[i].obj);
                     m_DoorWait.Reset();

                     pResultGoal->dir   = m_pAIState->GetFacingAng();
                     pResultGoal->dest  = *m_pAIState->GetLocation();
                     pResultGoal->speed = kAIS_Stopped;
                     return TRUE;
                  }
                  else if (fIsOpen)
                  {
                     int doorflags = 0;
                     if (fWasAIOpened) doorflags |= kWasAIOpened;
                     if (fWasAIUnlocked) doorflags |= kWasAIUnlocked;
                     g_pAIDoorLinks->AddFull(GetID(), doors[i].obj, &doorflags);
                     return TRUE;
                  }
               }
            }
         }
      }
   }
   return FALSE; // didn't change the goal
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDoorMovReg::WantsInterrupt()
{
   if (m_fWaiting && m_DoorWait.Expired())
   {
      m_fWaiting = FALSE;
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
