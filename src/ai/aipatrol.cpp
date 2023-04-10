/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipatrol.cpp,v 1.25 1999/12/22 13:13:45 BFarquha Exp $
//
// @TBD (toml 05-24-98): reconsider should patrol bit
//
//

#include <lg.h>
#include <appagg.h>
#include <hashpp.h>

#include <scrptapi.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
#include <propbase.h>
#include <propobsv.h>

#include <aiactloc.h>
#include <aiapiiai.h>
#include <aidfndpt.h>
#include <aidist.h>
#include <aigoal.h>
#include <aitrace.h>
#include <aipatrol.h>
#include <aiprabil.h>
#include <aiscrm.h>
#include <aitagtyp.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

IRelation * g_pAIPatrolLinks;
IRelation * g_pAICurPatrolLinks;

static cPropertyObserver g_AIPatrolPropObs;
cPropertyObserver * g_pAIPatrolPropObs = &g_AIPatrolPropObs;

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitPatrolAbility(IAIManager *)
{
   sRelationDesc     patrolDesc     = { "AIPatrol",        kRelationNetworkLocalOnly, 0, 0 };
   sRelationDesc     curPatrolDesc  = { "AICurrentPatrol", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc noDataDesc     = { "None", 0 };

   g_pAIPatrolLinks    = CreateStandardRelation(&patrolDesc,    &noDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));
   g_pAICurPatrolLinks = CreateStandardRelation(&curPatrolDesc, &noDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));

   g_AIPatrolPropObs.Init(g_pAIPatrolProperty, kPOF_DirtyRebuildConcrete);

   return TRUE;
}

///////////////////////////////////////

BOOL AITermPatrolAbility()
{
   SafeRelease(g_pAIPatrolLinks);
   SafeRelease(g_pAICurPatrolLinks);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Link graph helper
//

typedef cHashTable<ObjID, BOOL, cHashTableFunctions<ObjID> > cVisitedTable;

static void DoGatherGraphNodes(IRelation * pRelation, ObjID current, cVisitedTable & visits, cDynArray<ObjID> * pResult)
{
   cDynArray_<ObjID, 32> descends;
   AutoIPtr(LinkQuery);
   BOOL  dummy;
   sLink link;
   int i;

   Assert_(!visits.Lookup(current, &dummy));

   pResult->Append(current);
   visits.Insert(current, TRUE);

   pLinkQuery = pRelation->Query(current, LINKOBJ_WILDCARD);

   while (!pLinkQuery->Done())
   {
      pLinkQuery->Link(&link);
      descends.Append(link.dest);
      pLinkQuery->Next();
   }
   SafeRelease(pLinkQuery);

   for (i = 0; i < descends.Size(); i++)
      if (!visits.Lookup(descends[i], &dummy))
         DoGatherGraphNodes(pRelation, descends[i], visits, pResult);

   descends.SetSize(0);

   pLinkQuery = pRelation->Query(LINKOBJ_WILDCARD, current);

   while (!pLinkQuery->Done())
   {
      pLinkQuery->Link(&link);
      descends.Append(link.source);
      pLinkQuery->Next();
   }
   SafeRelease(pLinkQuery);

   for (i = 0; i < descends.Size(); i++)
      if (!visits.Lookup(descends[i], &dummy))
         DoGatherGraphNodes(pRelation, descends[i], visits, pResult);

}

static void GatherGraphNodes(IRelation * pRelation, ObjID start, cDynArray<ObjID> * pResult)
{
   cVisitedTable visits;
   DoGatherGraphNodes(pRelation, start, visits, pResult);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPatrol
//

cAIPatrol::cAIPatrol()
 : m_CheckPropTimer(kAIT_1Sec)
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIPatrol::GetName()
{
   return "Patrol ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPatrol::Init()
{
   cAINonCombatAbility::Init();
   SetNotifications(kAICN_UpdateProps | kAICN_GoalProgress | kAICN_Defend);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPatrol::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Patrol, 0, 1, pTagFile))
   {
      BaseAbilitySave(pTagFile);
      m_CheckPropTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPatrol::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Patrol, 0, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      m_CheckPropTimer.Load(pTagFile);
      AICloseTagBlock(pTagFile);
   }

   return TRUE;
}

///////////////////////////////////////

#define kPatrolMoveResetDistSq 1.0
// @TBD (toml 04-23-98): make sure this is fully rigged up
// @TBD (toml 04-24-98): reinstitute kPatrolMoveResetDistSq
STDMETHODIMP_(void) cAIPatrol::OnSimStart()
{
   cAINonCombatAbility::OnSimStart();
   m_CheckPropTimer.Force();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPatrol::OnUpdateProperties(eAIPropUpdateKind kind)
{
   if (g_AIPatrolPropObs.Touched() || m_CheckPropTimer.Expired())
   {
      SetShouldPatrolFromProperty();
      m_CheckPropTimer.Reset();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPatrol::OnGoalProgress(const cAIGoal * pGoal)
{
   cAINonCombatAbility::OnGoalProgress(pGoal);

   if (!pGoal->InProgress())
   {
      BOOL fPatrolGoal = IsOwn(pGoal);

      if (!fPatrolGoal || pGoal->result == kAIR_Success)
      {
         if (fPatrolGoal) // must be success
         {
            sAIPatrolPointMsg * pMsg = new sAIPatrolPointMsg(GetID(), pGoal->object);
            AutoAppIPtr(ScriptMan);
            pScriptMan->PostMessage(pMsg);
            pMsg->Release();
         }
         SignalGoal();
      }

      else if (fPatrolGoal)
      {
         // We failed for some reason: bail
#ifndef SHIP
         mprintf("WARNING: AI %d failed to get to patrol point %d\n", pGoal->object);
#endif
         Stop();
      }
   }
   //mprintf("Goal progress %d\n", pGoal->result);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPatrol::OnModeChange(eAIMode previous, eAIMode mode) // Inherits notification bit from base ability (toml 05-24-98)
{
   cAINonCombatAbility::OnModeChange(previous, mode);

   // As SetShouldPatrolFromProperty() is a sim-time function that
   // actually looks for patrol targets, it is not called in
   // Init(). If the AI was asleep or efficient, this object
   // was not receiving property notifications. So, when we
   // wake up, we make sure we're in sync...
   if (previous < kAIM_Efficient)
   {
      SetShouldPatrolFromProperty();
      m_CheckPropTimer.Reset();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPatrol::OnDefend(const sAIDefendPoint * pPoint)
{
   SignalGoal();
}

///////////////////////////////////////

#define kPatrolXYAccuracy    3.0
#define kPatrolZAccuracy     6.0

STDMETHODIMP cAIPatrol::SuggestGoal(cAIGoal * pCurrentGoal, cAIGoal ** ppGoal)
{
   ObjID      target = OBJ_NULL;
   cMxsVector loc;

   *ppGoal = NULL;

   if (AIGetDefendPoint(GetID()))
      return S_FALSE;

   if (m_pAIState->GetMode() > kAIM_Normal)
      return S_FALSE;

   if (!ShouldPatrol())
   {
      Stop();
      return S_FALSE;
   }

#ifndef SHIP
   if (pCurrentGoal && pCurrentGoal->object != GetCurrentPatrolObj())
      Warning(("AI %d current patrol link (%d) is not equal to patrol goal (%d)\n", GetID(), GetCurrentPatrolObj(), pCurrentGoal->object));
#endif

   // If this is the first time in to suggest goal, or we've come in with an
   // incomplete goal...
   if (!pCurrentGoal || pCurrentGoal->InProgress())
   {
      // ...then use any previous target link (usually from save game or
      // from designer)
      target = GetCurrentPatrolObj();
   }

   if (target == OBJ_NULL)
      target = TargetNextPatrolObj();

   if (target != OBJ_NULL)
   {
      cAIGotoGoal *  pGoal = new cAIGotoGoal(this);

      pGoal->priority = kAIP_Low;
      pGoal->object   = target;

      pGoal->LocFromObj();

      pGoal->accuracySq = sq(kPatrolXYAccuracy);
      pGoal->accuracyZ  = kPatrolZAccuracy;

      *ppGoal = pGoal;

      SignalAction();
      //mprintf("New goal: %d\n", target);
   }

   return (*ppGoal) ? S_OK : S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cAIPatrol::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAILocoAction * pAction = CreateLocoAction();
   pAction->SetFromGoal((cAIGotoGoal *)pGoal);
   pNew->Append(pAction);
   //mprintf("Suggest action");
   return S_OK;
}

///////////////////////////////////////

void cAIPatrol::SetShouldPatrolFromProperty()
{
   BOOL shouldPatrol = AIShouldPatrol(GetID());
   BOOL current = !!(m_flags & kShouldPatrol);

   if (shouldPatrol != current)
   {
      if (shouldPatrol)
      {
         m_flags |= kShouldPatrol;
         Start();
      }
      else
      {
         m_flags &= ~kShouldPatrol;
         Stop();
      }
      SignalGoal();
   }
}

///////////////////////////////////////

void cAIPatrol::Start()
{
   AITraceMsg(2, Patrol, "Starting patrol");
   //mprintf("Starting patrol\n");

   Assert_(ShouldPatrol());

// @TBD (toml 01-11-98):    UnSuspend();
//   if (!ShouldPatrol())
//      SetShouldPatrol(TRUE);
//   TargetNextPatrolObj();
   SignalGoal();
}

///////////////////////////////////////

void cAIPatrol::SetShouldPatrol(BOOL shouldPatrol)
{
   if (shouldPatrol)
      m_flags |= kShouldPatrol;
   else
      m_flags &= ~kShouldPatrol;

   if (shouldPatrol != AIShouldPatrol(GetID()))
      g_pAIPatrolProperty->Set(GetID(), shouldPatrol);
}

///////////////////////////////////////

void cAIPatrol::Stop()
{
   AITraceMsg(2, Patrol, "Stopping patrol");
   //mprintf("Stopping patrol\n");

   LinkID linkId = g_pAICurPatrolLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   if (linkId)
      g_pAICurPatrolLinks->Remove(linkId);

//   if (ShouldPatrol())
//      SetShouldPatrol(FALSE);

// @TBD (toml 02-25-98): Suspend();
}

///////////////////////////////////////

ObjID cAIPatrol::GetCurrentPatrolObj()
{
   LinkID linkId = g_pAICurPatrolLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   sLink  link;

   if (!linkId)
      return OBJ_NULL;

   g_pAICurPatrolLinks->Get(linkId, &link);

   return link.dest;
}

///////////////////////////////////////
//
// Pick a next patrol point and set that as the goal
//

ObjID cAIPatrol::TargetNextPatrolObj()
{
   sLink  link;
   ObjID  current = OBJ_NULL;
   ObjID  next    = OBJ_NULL;

   AITraceMsg(2, Patrol, "TargetNextPatrolObj");
   //mprintf("TargetNextPatrolObj\n");

   //
   // See if the AI has a next patrol obj, if not search for the closest patrol object
   //

   LinkID linkId = g_pAICurPatrolLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (linkId)
   {
      g_pAICurPatrolLinks->Get(linkId, &link);
      current = link.dest;
      g_pAICurPatrolLinks->Remove(linkId);
   }

   AutoIPtr(LinkQuery);
   if (current != OBJ_NULL)
   {
      if (!AIPatrolsRandom(GetID()))
      {

         // We have to count first, to find out how many branches there are...
         int count = 0;
         int target;

         pLinkQuery = g_pAIPatrolLinks->Query(current, LINKOBJ_WILDCARD);

         while (!pLinkQuery->Done())
         {
            pLinkQuery->Next();
            count++;
         }

         SafeRelease(pLinkQuery);

         if (!count)
         {
#ifndef SHIP
            mprintf("WARNING: AI %d: Reached end of patrol chain (obj %d). Stopping patrolling.\n", GetID(), current);
#endif
            Stop();
            SetShouldPatrol(FALSE);
            return OBJ_NULL;
         }

         // Now randomly pick a branch, and set that as the target
         target = AIRandom(0, count - 1);
         pLinkQuery = g_pAIPatrolLinks->Query(current, LINKOBJ_WILDCARD);

         for(count = 0; !pLinkQuery->Done(); pLinkQuery->Next(), count++)
         {
            if (count == target)
            {
               pLinkQuery->Link(&link);
               next = link.dest;
               break;
            }
         }
         SafeRelease(pLinkQuery);
      }
      else
      {
         // Random patrolling

         cDynArray<ObjID> choices;
         GatherGraphNodes(g_pAIPatrolLinks, current, &choices);

         if (!(choices.Size() - 1))
         {
#ifndef SHIP
            mprintf("WARNING: AI %d: Reached end of patrol chain (obj %d). Stopping patrolling.\n", GetID(), current);
#endif
            Stop();
            SetShouldPatrol(FALSE);
            return OBJ_NULL;
         }

         next = choices[AIRandom(1, choices.Size() - 1)];
      }
   }

   //
   // If we didn't have a patrol obj already, search for the closest
   //
   if (next == 0)
   {
      float      distSq,
                 minDistSq = kFloatMax;
      cMxsVector sourceLoc;
      const cMxsVector & loc = *m_pAIState->GetLocation();

      pLinkQuery = g_pAIPatrolLinks->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD);

      for (; !pLinkQuery->Done(); pLinkQuery->Next())
      {
         pLinkQuery->Link(&link);
         GetObjLocation(link.source, &sourceLoc);
         distSq = AIDistanceSq(sourceLoc, loc);

         if (distSq < minDistSq)
         {
            minDistSq = distSq;
            next = link.dest;
         }
      }
      SafeRelease(pLinkQuery);

   }

   //
   // If we have a patrol obj, head towards it.
   //
   if (next != OBJ_NULL)
   {
      g_pAICurPatrolLinks->Add(GetID(), next);
   }
   else
   {
#ifndef SHIP
      mprintf("WARNING: AI %d: cAIPatrol::TargetNextPatrolObj: No patrol objects found. Stopping patrolling\n", GetID());
#endif
      Stop();
      SetShouldPatrol(FALSE);
   }

   return next;
}

///////////////////////////////////////////////////////////////////////////////
