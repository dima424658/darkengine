/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiwatch.cpp,v 1.21 2000/02/26 15:35:57 toml Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <appagg.h>

#include <link.h>
#include <linkbase.h>
#include <linkman.h>
#include <lnkquery.h>
#include <playrobj.h>
#include <propface.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <iobjnet.h>

#include <aiactps.h>
#include <aiapiact.h>
#include <aiapinet.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aigoal.h>
#include <aipsdscr.h>
#include <aiprops.h>
#include <aitagtyp.h>
#include <aiwatch.h>
#include <aiwtctyp.h>
#include <aiutils.h>

#include <psdialog.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAIWatchPointProperty);

///////////////////////////////////////////////////////////////////////////////

static IRelation *             g_pAIWatchLinks;
static IAIWatchPointProperty * g_pAIWatchPointProperty;

///////////////////////////////////////////////////////////////////////////////
//
// Watch point property
//

#undef INTERFACE
#define INTERFACE IAIWatchPointProperty
DECLARE_PROPERTY_INTERFACE(IAIWatchPointProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIWatchPoint *);
};

///////////////////

static sAIWatchPoint _g_AIDefWatchPoint;

///////////////////

static sPropertyDesc _g_WatchPointPropertyDesc =
{
   "AI_WtchPnt",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_UTILITY_CAT, "Watch: Watch link defaults" },
   kPropertyChangeLocally,
};

///////////////////

typedef cSpecificProperty<IAIWatchPointProperty, &IID_IAIWatchPointProperty, sAIWatchPoint *, cHashPropertyStore< cClassDataOps<sAIWatchPoint> > > cAIWatchPointPropertyBase;

class cAIWatchPointProperty : public cAIWatchPointPropertyBase
{
public:
   cAIWatchPointProperty()
      : cAIWatchPointPropertyBase(&_g_WatchPointPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIWatchPoint);
};

///////////////////

#define AIGetWatchPointDefaults(obj) AIGetProperty(g_pAIWatchPointProperty, (obj), &_g_AIDefWatchPoint)

///////////////////////////////////////////////////////////////////////////////
//
// Watch links
//

static const char * _g_ppszAIWatchKinds[] =
{
   "Player intrusion",
   "Self entry"
};

static const char * _g_ppszAIWatchKill[] =
{
   "Don't kill",
   "After trigger",
   "After completion"
};

static const char * _g_ppszAIWatchLineReq[] =
{
   "None",
   "Line of sight",
   "Raycast"
};

#define WATCH_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAIWatchPoint, acts[i-1])

static sFieldDesc _g_AIWatchPointFieldDesc[] =
{
   { "Watch kind",         kFieldTypeEnum,  FieldLocation(sAIWatchPoint, kind),              kFieldFlagNone, 0, FieldNames(_g_ppszAIWatchKinds) },
   { "Priority",                    kFieldTypeEnum,  FieldLocation(sAIWatchPoint, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },
   { "Trigger: Radius",             kFieldTypeInt,   FieldLocation(sAIWatchPoint, radius)                                                                  },
   { "         Height",             kFieldTypeInt,   FieldLocation(sAIWatchPoint, height)                                                                  },
   { "         Required awareness", kFieldTypeEnum,  FieldLocation(sAIWatchPoint, awarenessRequired), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "         Line requirement",   kFieldTypeEnum,  FieldLocation(sAIWatchPoint, lineRequired),      kFieldFlagNone, 0, FieldNames(_g_ppszAIWatchLineReq) },
   { "         Minimum alertness",  kFieldTypeEnum,  FieldLocation(sAIWatchPoint, minAlert), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "         Maximum alertness",  kFieldTypeEnum,  FieldLocation(sAIWatchPoint, maxAlert), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },

   { "Exit: Link kill option",   kFieldTypeEnum,  FieldLocation(sAIWatchPoint, kill),              kFieldFlagNone, 0, FieldNames(_g_ppszAIWatchKill) },
   { "      Kill like links",    kFieldTypeBool, FieldLocation(sAIWatchPoint, fKillOthers)                                                            },

   { "      No test once triggered",kFieldTypeBool,  FieldLocation(sAIWatchPoint, fAlwaysComplete)                                                             },

   { "      Reuse delay",        kFieldTypeInt,   FieldLocation(sAIWatchPoint, reuseDelay)                                                                  },
   { "      Reset delay",        kFieldTypeInt,   FieldLocation(sAIWatchPoint, resetDelay)                                                                  },

   // @Note (toml 07-17-98): only showing 7 of 8 fields
   WATCH_RESPONSE_FIELDS(1),
   WATCH_RESPONSE_FIELDS(2),
   WATCH_RESPONSE_FIELDS(3),
   WATCH_RESPONSE_FIELDS(4),
   WATCH_RESPONSE_FIELDS(5),
   WATCH_RESPONSE_FIELDS(6),
   WATCH_RESPONSE_FIELDS(7),
};

static sStructDesc _g_AIWatchPointStructDesc = StructDescBuild(sAIWatchPoint, kStructFlagNone, _g_AIWatchPointFieldDesc);

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitWatchAbility(IAIManager *)
{
   sRelationDesc     watchPointDesc     = { "AIWatchObj", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc watchPointDataDesc = { "sAIWatchPoint", sizeof(sAIWatchPoint), kRelationDataAutoCreate };

   StructDescRegister(&_g_AIWatchPointStructDesc);

   g_pAIWatchLinks = CreateStandardRelation(&watchPointDesc, &watchPointDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));
   g_pAIWatchLinks->Listen(kListenLinkBirth | kListenLinkDeath, cAIWatch::LinkListenFunc, 0);

   g_pAIWatchPointProperty = new cAIWatchPointProperty;

   _g_AIDefWatchPoint.maxAlert = kAIAL_High;

   return TRUE;
}

///////////////////////////////////////

BOOL AITermWatchAbility()
{
   SafeRelease(g_pAIWatchLinks);
   SafeRelease(g_pAIWatchPointProperty);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

static cDynArray<cAIWatch *> g_AIWatchAbilities;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWatch -- watch component
//

cAIWatch::cAIWatch()
 : m_Timer(kAIT_1Sec),
   m_pWatch(NULL)
{
   g_AIWatchAbilities.Append(this);
}

///////////////////////////////////////

cAIWatch::~cAIWatch()
{
   for (int i = 0; i < g_AIWatchAbilities.Size(); i++)
   {
      if (g_AIWatchAbilities[i] == this)
      {
         g_AIWatchAbilities.FastDeleteItem(i);
         break;
      }
   }
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIWatch::GetName()
{
   return "Watch component";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIWatch::Init()
{
   SetNotifications(kAICN_GoalChange | kAICN_ActionProgress);
   m_Timer.Reset();
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIWatch::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Watch, 0, 0, pTagFile))
   {
      BaseAbilitySave(pTagFile);
      m_Timer.Save(pTagFile);

      LinkID linkID = (m_pWatch) ? m_pWatch->linkID : 0;

      AITagMove(pTagFile, &linkID);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIWatch::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Watch, 0, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      m_Timer.Load(pTagFile);

      m_pWatch = NULL;
      LinkID linkID;
      AITagMove(pTagFile, &linkID);
      if (linkID)
      {
         AutoAppIPtr(LinkManager);
         linkID = pLinkManager->RemapOnLoad(linkID);
         m_pWatch = (sAIWatchPoint *)g_pAIWatchLinks->GetData(linkID);
         Assert_(m_pWatch);
      }

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIWatch::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   if (IsOwn(pGoal))
      SignalAction();
   // else, if we didn't complete the last action
   else if (m_pWatch)
   {
      m_pWatch->nextStep = 0; // @TBD (toml 07-20-98): continue script even after goal override? not doing that right now
      StopCurrent();
   }
}

///////////////////////////////////////

DECLARE_TIMER(cAIWatch_OnActionProgress, Average);

STDMETHODIMP_(void) cAIWatch::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIWatch_OnActionProgress);

   // If we're in a good position to interrupt...
   if (pAction->GetResult() > kAIR_NoResult)
   {
      if (InControl() && m_pWatch)
      {
         if (pAction->GetResult() == kAIR_NoResultSwitch && !m_Timer.Expired())
         {
            return;
         }

         m_pWatch->fCompleted = !pAction->InProgress();
         m_pWatch->nextStep = (((cAIPsdScrAction *)pAction)->IsLast()) ?
                                 kAIStepsComplete :
                                 ((cAIPsdScrAction *)pAction)->GetCurrentStep() + 1;
      }
      else if (!m_Timer.Expired())
      {
         return;
      }

      m_Timer.Reset();
      SignalGoal();
   }
}

///////////////////////////////////////

STDMETHODIMP cAIWatch::SuggestGoal(cAIGoal * pCurrentGoal, cAIGoal ** ppGoal)
{
   sAIWatchPoint * pPrevWatch = m_pWatch;

   BOOL fNewWatch = SeekTarget();

   if (m_pWatch)
   {
      if (!fNewWatch)
      {
         pCurrentGoal->AddRef();
         *ppGoal = pCurrentGoal;
      }
      else
      {
         *ppGoal = new cAICustomGoal(this);
         if (!m_pWatch->priority)
            (*ppGoal)->priority = kAIP_Normal;
         else
            (*ppGoal)->priority = m_pWatch->priority;
      }
   }
   else
   {
      *ppGoal = NULL;
   }

   return (*ppGoal) ? S_OK : S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cAIWatch::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   if (m_pWatch)
   {
      cAIPsdScrAction * pAction = CreatePsdScrAction();

      pAction->Set(m_pWatch->acts,
                   kAIMaxWatchActs,
                   (m_pWatch->resetTime < AIGetTime()) ? 0 : m_pWatch->nextStep);
      pAction->SetLink(GetID(), m_pWatch->object);

      pNew->Append(pAction);

      m_Timer.Reset();
   }
   else // Someone edited the watch link, possibly as a result of a script message dispatch since the last SuggestGoal()
      SignalGoal();

   return S_OK;
}

///////////////////////////////////////
//
// Finds the "most interesting" watch link.
//
// Returns TRUE of there's a change of target
//

#define kMaxSpeed 20

BOOL cAIWatch::SeekTarget()
{
   AutoIPtr(LinkQuery);
   sAIWatchPoint *       pResult    = NULL;
   float                 bestDistSq = kFloatMax;
   int                   bestRadius = INT_MAX;
   sAIWatchPoint *       pCurrent;
   sLink                 link;
   BOOL                  changed;

   // If we have a watch and
   if (m_pWatch)
   {
      // And it's done, clear it
      if (m_pWatch->fCompleted)
         StopCurrent();
      else if (m_pWatch->fAlwaysComplete)
         return FALSE;
   }

   // If we have an active watch, we always start with that
   if (m_pWatch)
   {
      if (CheckTrigger(m_pWatch))
      {
         // If the others have been "killed" (in name) and we're still doing a
         // valid watch, just continue what we're doing
         if (m_pWatch->fKillOthers)
            return FALSE;

         bestDistSq = m_pWatch->distSq;
         bestRadius = m_pWatch->radius;
         pResult = m_pWatch;
      }
   }

   pLinkQuery = g_pAIWatchLinks->Query(GetID(), (m_pWatch) ? m_pWatch->object : LINKOBJ_WILDCARD);

   for(; !pLinkQuery->Done(); pLinkQuery->Next())
   {
      pLinkQuery->Link(&link);
      pCurrent = (sAIWatchPoint * )pLinkQuery->Data();

      // Current watch handled specially above
      if (m_pWatch == pCurrent)
         continue;

      if (pCurrent->reuseTime > AIGetTime())
         continue;

      pCurrent->object = link.dest;

      if (CheckTrigger(pCurrent))
      {
         if (pCurrent->distSq > bestDistSq)
            continue;

         if (pCurrent->distSq == bestDistSq && pCurrent->radius > bestRadius)
            continue;

         bestDistSq = pCurrent->distSq;
         bestRadius = pCurrent->radius;
         pResult = pCurrent;

         pResult->linkID = pLinkQuery->ID();
      }
      else if ((int)pCurrent->distSq > sq(pCurrent->radius + kMaxSpeed))
      {
         // if the trigger fails, and the target is at least as far away as can be
         // travelled in a second (the defaultrefresh rate) delay until even
         // possible to trigger
         float distOutside = sqrt(pCurrent->distSq) - (float)(pCurrent->radius);

         unsigned minEntryTime = (unsigned)((distOutside / (float)(kMaxSpeed)) * 1000.0) + AIGetTime();
         if (minEntryTime > pCurrent->reuseTime)
            pCurrent->reuseTime = minEntryTime;
      }
   }

   if (!pResult)                                 // If there is no match
   {
      changed = !!m_pWatch;
      if (changed)
         StopCurrent();
   }
   else
   {
      changed = (m_pWatch != pResult);
      if (changed && m_pWatch)
      {
         m_pWatch->nextStep = 0;                    // is this the right behavior? (toml 07-22-98)
         StopCurrent();
      }
      m_pWatch = pResult;
   }

   return changed;
}

///////////////////////////////////////

BOOL cAIWatch::CheckTrigger(sAIWatchPoint * pWatch)
{
   BOOL  result = FALSE;

   eAIAlertLevel alertness = m_pAIState->GetAlertness();
   if (alertness < pWatch->minAlert ||
       alertness > pWatch->maxAlert)
   {
      return FALSE;
   }

   if (pWatch->kind == kAIWK_Player)
   {
      if (!g_fAIAwareOfPlayer)
         return FALSE;

      AutoAppIPtr(ObjectNetworking);
      if (pObjectNetworking->ObjLocalOnly(GetID()))
      {
         // Local-only AIs only watch for the local player:
         if (CheckTriggerForObj(pWatch, PlayerObject(), PlayerObject()))
            return TRUE;
      } else {
         AutoAppIPtr(AINetServices);
         tAIPlayerList players;

         pAINetServices->GetPlayers(&players);

         ObjID * pPlayerID = &players[0];

         while (*pPlayerID)
         {
            if (CheckTriggerForObj(pWatch, *pPlayerID, *pPlayerID))
               return TRUE;
            pPlayerID++;
         }
      }

      return FALSE;
   }
   else
   {
      return CheckTriggerForObj(pWatch, GetID(), pWatch->object);
   }
}

///////////////////////////////////////

BOOL cAIWatch::CheckTriggerForObj(sAIWatchPoint * pWatch, ObjID obj, ObjID senseObj)
{
   mxs_vector            objLoc;
   mxs_vector            watchLoc;
   const sAIAwareness  * pAwareness;

   GetObjLocation(obj, &objLoc);
   GetObjLocation(pWatch->object, &watchLoc);

   if (AIInsideCylinder(objLoc, watchLoc, sq(pWatch->radius), pWatch->height / 2, &pWatch->distSq))
   {
      if (pWatch->awarenessRequired || pWatch->lineRequired > kAIWLR_None)
      {
         pAwareness = m_pAI->GetAwareness(senseObj);

         if (pWatch->lineRequired == kAIWLR_LOS)
         {
            if (!(pAwareness->flags & kAIAF_HaveLOS))
               return FALSE;
         }
         else if (pWatch->lineRequired == kAIWLR_Raycast)
         {
            if (!(pAwareness->flags & kAIAF_CanRaycast))
               return FALSE;
         }

         if (pAwareness->level < pWatch->awarenessRequired)
            return FALSE;
      }

      pWatch->center     = watchLoc;
      pWatch->triggerObj = obj;

      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////

void cAIWatch::StopCurrent()
{
   ObjID  object       = m_pWatch->object;
   BOOL   fKillOthers  = m_pWatch->fKillOthers;
   BOOL   fKillCurrent = FALSE;
   LinkID currentID    = m_pWatch->linkID;

   if ((m_pWatch->fCompleted && m_pWatch->kill == kAIWK_OnComplete) ||
       m_pWatch->kill == kAIWK_OnTrigger)
   {
      fKillCurrent = TRUE;
   }
   else
   {
      if (m_pWatch->resetDelay && m_pWatch->nextStep != kAIStepsComplete)
      {
         m_pWatch->resetTime = AIGetTime() + m_pWatch->resetDelay;
      }
      else
      {
         m_pWatch->nextStep = 0;
         m_pWatch->resetTime = 0;
      }

      if (m_pWatch->reuseDelay)
         m_pWatch->reuseTime = AIGetTime() + m_pWatch->reuseDelay;
      else
         m_pWatch->reuseTime = 0;
   }

   gm_fSelfLinkEdit = TRUE;

   if (fKillCurrent)
   {
      g_pAIWatchLinks->Remove(m_pWatch->linkID);
      // m_pWatch is now invalid
   }

   m_pWatch = NULL;

   if (fKillOthers)
   {
      AutoIPtr(LinkQuery);
      pLinkQuery = g_pAIWatchLinks->Query(GetID(), object);
      for(; !pLinkQuery->Done(); pLinkQuery->Next())
      {
         if (!fKillCurrent && currentID == pLinkQuery->ID())
            continue;
         g_pAIWatchLinks->Remove(pLinkQuery->ID());
      }
   }

   gm_fSelfLinkEdit = FALSE;
}

///////////////////////////////////////

BOOL cAIWatch::gm_fSelfLinkEdit;

void LGAPI cAIWatch::LinkListenFunc(sRelationListenMsg* pMsg, RelationListenerData data)
{
   if (pMsg->type == kListenLinkDeath)
   {
      for (int i = 0; i < g_AIWatchAbilities.Size(); i++)
      {
         if (g_AIWatchAbilities[i]->GetID() == pMsg->link.source)
         {
            if (g_AIWatchAbilities[i]->m_pWatch && g_AIWatchAbilities[i]->m_pWatch->linkID == pMsg->id)
            {
               g_AIWatchAbilities[i]->m_pWatch = NULL;
               g_AIWatchAbilities[i]->SignalGoal();
            }
            return;
         }
      }
   }
   else if (pMsg->type == kListenLinkBirth)
   {
      g_pAIWatchLinks->SetData(pMsg->id, (void *)AIGetWatchPointDefaults(pMsg->link.dest));
   }
}

///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The new WatchObj dialog, with a list and sub-dialogs rather than a single huge one
//

static sFieldDesc gWatchHeaderFieldDesc[] =
{
   { "Watch kind",         kFieldTypeEnum,  FieldLocation(sAIWatchPoint, kind),              kFieldFlagNone, 0, FieldNames(_g_ppszAIWatchKinds) },
   { "Trigger: Radius",             kFieldTypeInt,   FieldLocation(sAIWatchPoint, radius)                                                                  },
   { "         Height",             kFieldTypeInt,   FieldLocation(sAIWatchPoint, height)                                                                  },
   { "         Required awareness", kFieldTypeEnum,  FieldLocation(sAIWatchPoint, awarenessRequired), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "         Line requirement",   kFieldTypeEnum,  FieldLocation(sAIWatchPoint, lineRequired),      kFieldFlagNone, 0, FieldNames(_g_ppszAIWatchLineReq) },
   { "         Minimum alertness",  kFieldTypeEnum,  FieldLocation(sAIWatchPoint, minAlert), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "         Maximum alertness",  kFieldTypeEnum,  FieldLocation(sAIWatchPoint, maxAlert), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },

   { "Exit: Link kill option",   kFieldTypeEnum,  FieldLocation(sAIWatchPoint, kill),              kFieldFlagNone, 0, FieldNames(_g_ppszAIWatchKill) },
   { "      Kill like links",    kFieldTypeBool, FieldLocation(sAIWatchPoint, fKillOthers)                                                            },

   { "      No test once triggered",kFieldTypeBool,  FieldLocation(sAIWatchPoint, fAlwaysComplete)                                                             },

   { "      Reuse delay",        kFieldTypeInt,   FieldLocation(sAIWatchPoint, reuseDelay)                                                                  },
   { "      Reset delay",        kFieldTypeInt,   FieldLocation(sAIWatchPoint, resetDelay)                                                                  },
};
static sStructDesc gWatchHeaderStructDesc = StructDescBuild(sAIWatchPoint, kStructFlagNone, gWatchHeaderFieldDesc);

IStructEditor* NewWatchObjDialog (void* data)
{
   return NewPseudoScriptDialog ("Watch obj responses", kAIMaxWatchActs, &gWatchHeaderStructDesc, data, ((sAIWatchPoint*) data)->acts);
}

#endif
