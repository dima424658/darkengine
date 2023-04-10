/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiflee.cpp,v 1.25 2000/03/16 14:17:16 bfarquha Exp $
//
// @TBD (toml 07-14-98): right now, no flee links expire as soon as AI moves on
//

// #define PROFILE_ON 1

#include <lg.h>

#include <dmgbase.h>
#include <link.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <mtagvals.h>
#include <objhp.h>
#include <playrobj.h>
#include <propface.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <weapcb.h>

#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aidebug.h>
#include <aiflee.h>
#include <aigoal.h>
#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactori.h>
#include <aiactinv.h>
#include <aiprops.h>
#include <aiprcore.h>
#include <aisndtyp.h>
#include <aitagtyp.h>
#include <aiteams.h>

#include <aipathdb.h>
#include <aipthcst.h>

#include <aiinform.h>

#include <dbmem.h>

// wsf: added code so that an AI won't try to

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIFleeConditionsProperty);

static IRelation *                 g_pAIFleeSourceLinks;
static IRelation *                 g_pAIFleeDestLinks;
static IRelation *                 g_pAINoFleeLinks;

// Designer-defined links from AI's to flee points.
static IRelation *                 g_pAIFleeToLinks;

static IAIFleeConditionsProperty * g_pAIFleeConditionsProperty;
// This added because the ConditionsProperty version is 0, which implicitly disallows bumping
// up to 1 and still being able to load data. We're in patch now, and don't want to make the designers
// re-instantiate the AIs. Let's just make a small adjunct property.
static IBoolProperty             * g_pAIFleeConditionsAwarenessProperty = NULL;

IIntProperty *              g_pAIFleePointProperty = 0;


///////////////////////////////////////////////////////////////////////////////
//
// Link/Property structures
//

struct sAIFleeDest
{
   ObjID    object;
   BOOL     fReached;
   unsigned expiration;
};

///////////////////////////////////////

struct sAINoFleeLink
{
   unsigned expiration;
};

///////////////////////////////////////

enum eAIFleeConditions
{
   kAIFC_Never,
   kAIFC_LowAlertness,
   kAIFC_ModAlertness,
   kAIFC_HighAlertness,
   kAIFC_Hitpoints,
   kAIFC_Threat,
   kAIFC_Frustrated,

   kAIFC_Num,

   kAIFC_IntMax = 0xffffffff
};

///////////////////

struct sAIFleeConditions
{
   eAIFleeConditions conditions;
   int               pctHitpoints;
   int               friends;
};

///////////////////////////////////////
//
// Flee property
//

#undef INTERFACE
#define INTERFACE IAIFleeConditionsProperty
DECLARE_PROPERTY_INTERFACE(IAIFleeConditionsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIFleeConditions *);
};

///////////////////

static sAIFleeConditions _g_AIDefFleeConditions =
{
   kAIFC_Never,
   0
};

///////////////////

static const char * _g_ppszAIFleeConditions[kAIFC_Num] =
{
   "Never",
   "On low alert",
   "On moderate alert",
   "On high alert",
   "On low hitpoints",
   "On threat",
   "On Frustration"
};

///////////////////

static sFieldDesc _g_AIFleeConditionsFieldDesc[] =
{
   { "Condition",              kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIFC_Num, kAIFC_Num, _g_ppszAIFleeConditions },
   { "Pct Hitpoints",          kFieldTypeInt, FieldLocation(sAIFleeConditions, pctHitpoints) },
   { "Not if # of friends >=", kFieldTypeInt, FieldLocation(sAIFleeConditions, friends) },
};

static sStructDesc _g_AIFleeConditionsStructDesc = StructDescBuild(sAIFleeConditions, kStructFlagNone, _g_AIFleeConditionsFieldDesc);

///////////////////

static sPropertyDesc _g_FleePointPropertyDesc =
{
   "AI_FleeConds",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Flee: Condition for flee" },
   kPropertyChangeLocally, // net_flags
};

static sPropertyDesc _g_pAIFleeConditionsAwarenessDesc =
{
  "AI_FleeAwr",
  0,
  NULL, 0, 0,
  { AI_ABILITY_CAT, "Flee: If Aware of AI/Player" },
  kPropertyChangeLocally,
};

///////////////////

typedef cSpecificProperty<IAIFleeConditionsProperty, &IID_IAIFleeConditionsProperty, sAIFleeConditions *, cHashPropertyStore< cClassDataOps<sAIFleeConditions> > > cAIFleeConditionsPropertyBase;

class cAIFleeConditionsProperty : public cAIFleeConditionsPropertyBase
{
public:
   cAIFleeConditionsProperty()
      : cAIFleeConditionsPropertyBase(&_g_FleePointPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIFleeConditions);
};

///////////////////

#define AIGetFleeConditions(obj) AIGetProperty(g_pAIFleeConditionsProperty, (obj), &_g_AIDefFleeConditions)

///////////////////////////////////////
//
// Flee dest link
//

static sFieldDesc _g_AIFleeDestLinkFieldDesc[] =
{
   { "Reached", kFieldTypeBool, FieldLocation(sAIFleeDest, fReached) },
   { "Expiration", kFieldTypeInt, FieldLocation(sAIFleeDest, expiration) },
};

static sStructDesc _g_FleeDestLinkStructDesc = StructDescBuild(sAIFleeDest, kStructFlagNone, _g_AIFleeDestLinkFieldDesc);

///////////////////////////////////////
//
// No flee link
//

static sFieldDesc _g_AINoFleeLinkFieldDesc[] =
{
   { "Expiration", kFieldTypeInt, FieldLocation(sAINoFleeLink, expiration) },
};

static sStructDesc _g_NoFleeLinkStructDesc = StructDescBuild(sAINoFleeLink, kStructFlagNone, _g_AINoFleeLinkFieldDesc);

///////////////////////////////////////
//
// Flee point property
//

static sPropertyDesc sFleePointPropertyDesc =
{
   "AI_FleePoint",
   kPropertyInstantiate,
   NULL, 0, 0,  // constraints, version
   { AI_UTILITY_CAT, "Flee point" },
   kPropertyChangeLocally, // net_flags
};

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitFleeAbility(IAIManager *)
{
   //
   // Sdescs
   //
   StructDescRegister(&_g_AIFleeConditionsStructDesc);
   StructDescRegister(&_g_NoFleeLinkStructDesc);
   StructDescRegister(&_g_FleeDestLinkStructDesc);

   //
   // Links
   //
   sRelationDesc     fleeSourceDesc = { "AIFleeSource",  kRelationNetworkLocalOnly, 0, 0 };
   sRelationDesc     fleeDestDesc   = { "AIFleeDest",    kRelationNetworkLocalOnly, 0, 0 };
   sRelationDesc     noFleeDesc     = { "AINoFlee",      kRelationNetworkLocalOnly, 0, 0 };
   sRelationDesc     fleeToDesc     = { "AIFleeTo",      kRelationNetworkLocalOnly, 0, 0 };

   sRelationDataDesc noDataDesc       = { "None", 0 };
   sRelationDataDesc fleeDestDataDesc = { "sAIFleeDest", sizeof(sAIFleeDest), kRelationDataAutoCreate };
   sRelationDataDesc noFleeDataDesc   = { "sAINoFleeLink", sizeof(sAINoFleeLink), kRelationDataAutoCreate };


   // @TBD (toml 07-13-98): ask marc about what the right query opt flags should be here
   g_pAIFleeSourceLinks = CreateStandardRelation(&fleeSourceDesc, &noDataDesc,       (kQCaseSetSourceKnown|kQCaseSetDestKnown));
   g_pAIFleeDestLinks   = CreateStandardRelation(&fleeDestDesc,   &fleeDestDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));;
   g_pAINoFleeLinks     = CreateStandardRelation(&noFleeDesc,     &noFleeDataDesc,   (kQCaseSetSourceKnown|kQCaseSetDestKnown));;

   g_pAIFleeToLinks = CreateStandardRelation(&fleeToDesc, &noDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));

   //
   // Properties
   //
   g_pAIFleePointProperty      = CreateIntProperty(&sFleePointPropertyDesc, kPropertyImplVerySparse);
   g_pAIFleeConditionsProperty = new cAIFleeConditionsProperty;
   g_pAIFleeConditionsAwarenessProperty = CreateBoolProperty(&_g_pAIFleeConditionsAwarenessDesc, kPropertyImplSparse);

   return TRUE;
}

///////////////////////////////////////

BOOL AITermFleeAbility()
{
   SafeRelease(g_pAIFleeSourceLinks);
   SafeRelease(g_pAIFleeDestLinks);
   SafeRelease(g_pAINoFleeLinks);

   SafeRelease(g_pAIFleeToLinks);

   SafeRelease(g_pAIFleePointProperty);
   SafeRelease(g_pAIFleeConditionsProperty);
   SafeRelease(g_pAIFleeConditionsAwarenessProperty);

   return TRUE;
}



// Return TRUE if this Id is already someone's flee destination.
BOOL AIFleeIsCurrentDest(ObjID dest)
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(LINKOBJ_WILDCARD, dest);
   return !!linkId;
}



///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFlee
//

cAIFlee::cAIFlee()
 : m_CheckTimer(kAIT_2Sec),
   m_BroadcastTimer(7500, 25000),
   m_ExplicitDestId(0),
   nLastFailedGoalTime(-10000),
   bFiguringDest(FALSE),
   m_pPath(0),
   m_pExplicitPath(0),
   m_nNumFleePoints(0),
   bWasDamage(FALSE),
   nFleeSource(0),
   bDoingInvestigate(FALSE)
{
}


cAIFlee::~cAIFlee()
{
   if (m_pPath)
   {
      m_pPath->Release();
      m_pPath = 0;
   }

   if (m_pExplicitPath)
   {
      m_pExplicitPath->Release();
      m_pExplicitPath = 0;
   }

   g_pAIInformSeenFrom->Delete(GetID());
}


///////////////////////////////////////

STDMETHODIMP_(const char *) cAIFlee::GetName()
{
   return "Flee ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlee::Init()
{
   cAIAbility::Init();
   SetNotifications(kAICN_Alertness | kAICN_Awareness | kAICN_Damage | kAICN_GoalChange | kAICN_Weapon | kAICN_Frustration);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIFlee::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Flee, 0, 0, pTagFile))
   {
      BaseAbilitySave(pTagFile);

      m_CheckTimer.Save(pTagFile);
      m_BroadcastTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIFlee::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Flee, 0, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);

      m_CheckTimer.Load(pTagFile);
      m_BroadcastTimer.Load(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlee::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   if (IsOwn(pGoal))
   {
      SetNotifications(kAICN_ActionProgress);
      SignalAction();
   }
   else
   {
      ClearNotifications(kAICN_ActionProgress);
      StopFlee();
   }
}

///////////////////////////////////////

#define kMinFleeWait 10000
#define kMaxFleeWait 60000
#define kContinueFleeTime 1000
#define kFleeContinueRadiusSq sq(25.0)

DECLARE_TIMER(cAIFlee_OnActionProgress, Average);

STDMETHODIMP_(void) cAIFlee::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIFlee_OnActionProgress);

   Assert_(IsOwn(pAction));

   BOOL fInProgress = pAction->InProgress();
   BOOL fFleeDestReached = GetFleeDestReached();

   // If we've arrived at the flee point...
   if (!fInProgress && pAction->GetType() == kAIAT_Locomote && !fFleeDestReached)
   {
      // Mark appropriate links
      FlagFleeDestReached();
      fFleeDestReached = TRUE;
      SetFleeDestExpiration(AIRandom(kMinFleeWait, kMaxFleeWait));
      m_CheckTimer.Reset();
   }

   ObjID                fleeSource       = GetFleeSource();
   const sAIAwareness * pAwareness       = m_pAI->GetAwareness(fleeSource);
   unsigned             timeSinceContact = (pAwareness->flags & kAIAF_CanRaycast) ? 0 :
                                                                                    pAwareness->TimeSinceContact();


   // If we're aware of source and we're investigating, then run again.
   if ((pAwareness->level == kAIAL_High) &&
       (pAwareness->flags & kAIAF_CanRaycast) &&
       (pAction->GetType() == kAIAT_Investigate))
   {
      SignalAction();
      return;
   }

   // If we've been at the flee point for the specified time without any contact
   if (fFleeDestReached && GetFleeDestExpiration() < timeSinceContact)
   {
      StopFlee();
      return;
   }

   // Check for continued pursuit...
   if (fFleeDestReached && pAction->GetResult() > kAIR_NoResultSwitch)
   {
      mxs_vector sourceLoc;
      GetObjLocation(fleeSource, &sourceLoc);
      if (((pAwareness->flags & kAIAF_CanRaycast) || timeSinceContact < kContinueFleeTime) &&
          m_pAIState->DistSq(sourceLoc) < kFleeContinueRadiusSq)
      {
         SignalGoal();
         return;
      }
   }

   // If we're idle, signal for more actions
   if (!fInProgress)
   {
      SignalAction();
   }
   else if ((pAction->GetType() == kAIAT_Locomote) && (pAction->GetResult() == kAIR_NoResultSwitch)) // test if we've met "friends" condition. We'd also like to test path to player in cast of explicit path, but that's expensive.
   {
      if (m_CheckTimer.Expired())
      {
         sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());
         if (CheckFriendsNotFlee(pConditions))
            SignalGoal();
         m_CheckTimer.Reset();
      }
   }

   if (m_BroadcastTimer.Expired() && m_pAI->AccessSoundEnactor())
   {
      if (m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_ReactRun) == S_OK)
      {
         m_BroadcastTimer.Reset();
      }
   }
}

///////////////////////////////////////

// pass in object that is frustrating us.
STDMETHODIMP_(void) cAIFlee::OnFrustration(ObjID source, ObjID dest, IAIPath *pPath)
{
   sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());

    if (CheckFriendsNotFlee(pConditions))
      return;

   if (GetFleeSource())
   {
      // if we're investigating. Stop doind that, and try to run away again.
      if (bDoingInvestigate)
         StopFlee();
      else
         return;
   }

   // Also resets m_ExplicitDestId and m_pExplicitPath
   SetFleeSource(source);

   // If dest has an AIInform property, set this:
   if (dest)
   {
      if (AIHasInformResponse(dest))
         g_pAIInformSeenFrom->Set(dest, TRUE);
   }

   m_ExplicitDestId = dest;
   m_pExplicitPath = pPath;
   SignalGoal();
}


///////////////////////////////////////

// If we currently have no source, then test alertness and damage to see if we want to flee
// This was added in an effort to get non-combatant AI's to flee better. Currently, there
// is a problem with non-combat AI's where when they lose contact with you, they'll never try
// to Flee again as long as their alertness is high. This is because OnAlert only fires when the
// alert level changes.
STDMETHODIMP_(void) cAIFlee::OnAwareness(ObjID source, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   if (!source || (source == GetID()))
      return;

   OnAlertness(source, current, current, pRaw);
   if (bWasDamage) // Ok, admittedly, this is somewhat hackly. We're just trying to preserve what our previous flee reason was, so non-combat AIs won't just investigate after losing 'source'.
   {
      sDamageMsgData MsgDat;
      MsgDat.kind = kDamageMsgImpact;
      sDamageMsg Msg(0,&MsgDat);
      OnDamage(&Msg, 0);
   }
}



STDMETHODIMP_(void) cAIFlee::OnAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   const sAIAwareness * pAwareness = m_pAI->GetAwareness(source);

   if (source == OBJ_NULL)
      return;

   static eAIAwareLevel fleeLevels[kAIFC_Num] =
   {
      kAIAL_Num,                                 // No flee
      kAIAL_Low,
      kAIAL_Moderate,
      kAIAL_High,
      kAIAL_Num,                                 // No flee
      kAIAL_Num,                                 // No flee
      kAIAL_Num,                                 // No flee
   };

   sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());

   if (CheckFriendsNotFlee(pConditions))
      return;

   // We we must be aware of opposing AI or player:
   BOOL bVal = FALSE;
   if (g_pAIFleeConditionsAwarenessProperty->Get(GetID(), &bVal) && bVal)
   {
      // If the source is a player or AI we care about, then test Line of Sight. If we don't have it, then fail.
      if ((IsAPlayer(source) || (AITeamCompare(AIGetTeam(source), AIGetTeam(GetID())) == kAI_Opponents)) &&
          (!pAwareness || !(pAwareness->flags & kAIAF_HaveLOS)))
         return;
   }


   if (current >= fleeLevels[pConditions->conditions] || CheckHitpointsFlee(pConditions))
   {
      if (GetFleeSource())
      {
         // if we're investigating. Stop doind that, and try to run away again.
         if (bDoingInvestigate)
            StopFlee();
         else
            return;
      }

      SetFleeSource(source);
      SignalGoal();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlee::OnDamage(const sDamageMsg * pMsg, ObjID realCulpritID)
{
   if (pMsg->kind != kDamageMsgDamage)
      return;

   const sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());

   if (pConditions->conditions == kAIFC_Hitpoints)
   {
      if (CheckFriendsNotFlee(pConditions))
         return;

      if (GetFleeSource())
      {
         // if we're investigating. Stop doind that, and try to run away again.
         if (bDoingInvestigate)
            StopFlee();
         else
            return;
      }

      if (CheckHitpointsFlee(pConditions))
      {
         // @TBD (toml 07-13-98): this needs to be rewritten when multi-AI combat goes in
         // Note (mtr) the closest player may not be the right thing to flee from,
         // but it shouldn't hurt.
         SetFleeSource(m_pAI->GetClosestPlayer());
         bWasDamage = TRUE;
         SignalGoal();
      }
   }
}


///////////////////////////////////////

STDMETHODIMP_(void) cAIFlee::OnWeapon(eWeaponEvent ev, ObjID victim, ObjID culprit)
{
   sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());
   if (pConditions->conditions == kAIFC_Threat &&
       culprit != GetID() &&
       AITeamCompare(AIGetTeam(culprit), AIGetTeam(GetID())) == kAI_Opponents)
   {
      if (CheckFriendsNotFlee(pConditions))
         return;

      if (GetFleeSource())
      {
         // if we're investigating. Stop doind that, and try to run away again.
         if (bDoingInvestigate)
            StopFlee();
         else
            return;
      }

      SetFleeSource(culprit);
      SignalGoal();
   }
}

///////////////////////////////////////


//
// wsf: one alternative to make this better is to pathfind first and store results, perhaps
// going ahead and create loco action. That way, if pathfind fails, we won't even suggest fleeing as
// a goal.
//

void cAIFlee::DoSuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   // new way: rely on Finalize
   cAIFleeGoal * pGoal = new cAIFleeGoal(this, AIGetTime());
   pGoal->priority = kAIP_High;
   pGoal->object = OBJ_NULL;
   pGoal->location = kInvalidLoc;
   *ppGoal = pGoal;
}


STDMETHODIMP cAIFlee::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   *ppGoal = NULL;

   // We do this since this could be a result of en-route friend checking.
   sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());
   if (CheckFriendsNotFlee(pConditions))
      return FALSE;

   ObjID source = GetFleeSource();
   int nCurrentTime = GetSimTime();

   if (source && ((nCurrentTime-nLastFailedGoalTime) > 8000))
      DoSuggestGoal(pPrevious, ppGoal);

   return S_OK;
}

///////////////////////////////////////

// We've been chosen as the goal to perform, now make sure we really want to do it:
STDMETHODIMP_(BOOL) cAIFlee::FinalizeGoal(cAIGoal *pGoal)
{
   const sAIFleeDest *pDest = GetFleeDest();
   if (pDest && pDest->object)
   {
      pGoal->object = pDest->object;
      pGoal->LocFromObj();
      return TRUE;
   }

   ObjID source = GetFleeSource();
   ObjID dest = PickFleePoint(source);
   int nCurrentTime = GetSimTime();

   // @TBD: If we fail to find a good dest, then maybe flee back to origin?
   if (dest)
   {
      SetFleeDest(dest);
      pGoal->object = dest;
      pGoal->LocFromObj();
      return TRUE;
   }
   else if (!bFiguringDest)
      nLastFailedGoalTime = nCurrentTime;

   return FALSE;
}



///////////////////////////////////////


void cAIFlee::ActSurprised(cAIActions * pNew)
{
   bDoingInvestigate = FALSE;
   cAIMotionAction * pSurpriseMotion = CreateMotionAction();
   pSurpriseMotion->AddTag(cTag("Discover", kMTV_set));
   pSurpriseMotion->AddTag(cTag("Recoil", kMTV_set));
   pNew->Append(pSurpriseMotion);
}


#define kFleeAccuracySq sq(6.0)


float g_FleeNoFreshenRange = 30.0;
float g_FleeSpeed = 11.0;


BOOL cAIFlee::RunAway(cAIActions *pNew, ObjID object, BOOL freshen)
{
   IAISenses * pSenses = m_pAI->AccessSenses();
   ObjID fleeSource = GetFleeSource();

   bDoingInvestigate = FALSE;
   cAILocoAction * pLocoAction = CreateLocoAction();
   pLocoAction->Set(object, kAIS_VeryFast, kFleeAccuracySq);
   // Do pathfind. See if dest is reachable at all. If not, then unset dest, and fail. Will give
   // AI opportunity to try some other goals other than just standing there.
   if (!m_pPath && !pLocoAction->Pathfind())
   {
      pLocoAction->Release();
      nLastFailedGoalTime = GetSimTime();
      FlagFleeDestReached();
      return FALSE;
   }
   else
   {
      if (m_pPath) // needs to be set. Pass it on to action, then we let go of it.
      {
         pLocoAction->SetPath(m_pPath);
         m_pPath->Release();
         m_pPath = 0;
      }
      else if (freshen)
      {
         float distKeepFreshSense = pLocoAction->ComputePathDist()  - g_FleeNoFreshenRange;

         if (distKeepFreshSense > 0.0)
            pSenses->KeepFresh(fleeSource, (distKeepFreshSense / g_FleeSpeed) * 1000.0);
      }
      pNew->Append(pLocoAction);
   }

   return TRUE;
}


STDMETHODIMP cAIFlee::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   IAISenses * pSenses = m_pAI->AccessSenses();
   ObjID fleeSource = GetFleeSource();
   BOOL freshen = (pSenses && IsAPlayer(fleeSource));
   const sAIAwareness * pAwareness = m_pAI->GetAwareness(fleeSource);

   if (freshen)
      pSenses->KeepFresh(fleeSource, 0);

   if (!GetFleeDest())
   {
      ObjID dest = PickFleePoint(fleeSource);
      if (dest)
         SetFleeDest(dest);
      else
         return S_FALSE;
   }

   // If we're just starting to flee...
   if (!previous.Size())
      ActSurprised(pNew);
   // Otherwise if we've not reached the flee point...
   else if (!GetFleeDestReached())
   {
      if (!RunAway(pNew, pGoal->object, freshen))
         return S_FALSE;
   }
   // On reaching spot, for now wheel around and hope we're facing player direction
   else if ((previous[0]->GetType() == kAIAT_Locomote && fleeSource) &&
            (pAwareness->level != kAIAL_High) || !(pAwareness->flags & kAIAF_CanRaycast))
   {
      cAIOrientAction * pOrientAction = CreateOrientAction();
      pOrientAction->Set(fleeSource);
      pNew->Append(pOrientAction);
      bDoingInvestigate = FALSE;
   }
   // Otherwise, if we're not aware of source, then start investigate, else try to run again.
   else if ((pAwareness->level != kAIAL_High) || !(pAwareness->flags & kAIAF_CanRaycast))
   {
      cAIInvestAction * pInvestAction = CreateInvestAction();
      pInvestAction->Set(pGoal->object);
      pInvestAction->SetDuration(AIRandom(10000, 20000));
      pNew->Append(pInvestAction);
      bDoingInvestigate = TRUE;
   }
   else // We're still aware of flee source; now try to run somewhere else. If we can't, then act surprised.
   {
      // We don't want to keep fleeing, though, if it's because we're trying to get to a friend.

      // Ideally, we'd do all this by resignaling, and getting past "surprised", and heading right
      // into running away. As it is, this code is a bit redundant.
      bDoingInvestigate = FALSE;
      StopFlee();

      if (m_ExplicitDestId)
         return S_FALSE;

      // See if we have enough friends nearby to make us not feel like fleeing anymore.
      sAIFleeConditions * pConditions = AIGetFleeConditions(GetID());
      if (CheckFriendsNotFlee(pConditions))
         return S_FALSE;

      ObjID dest = PickFleePoint(fleeSource);
      if (!dest) // No flee point, so don't do any flee actions.
         return S_FALSE;

      SetFleeSource(fleeSource);
      SetFleeDest(dest);
      if (!RunAway(pNew, pGoal->object, freshen))
         return S_FALSE;
   }

   return S_OK;
}

///////////////////////////////////////

BOOL cAIFlee::CheckHitpointsFlee(const sAIFleeConditions * pConditions)
{
   int hp    = 1;
   int maxHp = 1;

   ObjGetHitPoints(GetID(), &hp);
   ObjGetMaxHitPoints(GetID(), &maxHp);

   return ((hp * 100) / maxHp <= pConditions->pctHitpoints);
}


///////////////////////////////////////

BOOL cAIFlee::CheckFriendsNotFlee(const sAIFleeConditions * pConditions)
{
   if (m_ExplicitDestId || pConditions->friends == 0 || !m_pAI->AccessSenses())
      return FALSE;

   // Nearby friends
   cAIAwareArray awarenesses;
   #define kFriendClosenessSq sq(20.0)
   unsigned      nFriends = 0;

   m_pAI->AccessSenses()->GetAllAwareness(&awarenesses);
   for (int i = 0; i < awarenesses.Size(); i++)
   {
      if ((awarenesses[i]->flags & kAIAF_CanRaycast) &&
          AIGetMode(awarenesses[i]->object) != kAIM_Dead &&
          AITeamCompare(GetID(), awarenesses[i]->object) == kAI_Teammates)
      {
         float dist = m_pAIState->DistSq(*GetObjLocation(awarenesses[i]->object));
         if (dist < kFriendClosenessSq)
            nFriends++;
         break;
      }
   }

   return (nFriends >= pConditions->friends);
}

///////////////////////////////////////

BOOL cAIFlee::IsFleeing()
{
   return !!g_pAIFleeSourceLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
}

///////////////////////////////////////

ObjID cAIFlee::GetFleeSource()
{
   LinkID linkId = g_pAIFleeSourceLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   sLink  link;

   if (!linkId)
      return OBJ_NULL;

   g_pAIFleeSourceLinks->Get(linkId, &link);

   return link.dest;
}



// Given a flee point, see if it belongs to a region that has been marked unpathable.
BOOL cAIFlee::IsInUnpathableRegion(ObjID Obj)
{
   int nRegion = AIGetFleeRegion(Obj);

   if (nRegion < 0)
      return FALSE;

   return !!m_UnpathableRegions[nRegion];
}


void cAIFlee::MarkUnpathable(ObjID Obj)
{
   int nRegion = AIGetFleeRegion(Obj);

   if (nRegion < 0)
      return;

   m_UnpathableRegions[nRegion] = 1;
}




///////////////////////////////////////


#define ZBIAS 4.000
#define ZBIAS_THREASHHOLD 10.0

//
// We use a "threashhold" so that small differences in Z won't factor in. For example,
// if we have a flee point at floor level, but the AI's center is at 3 ft, then we don't want a
// biased z difference of 3*4=12 feet, which would easily put us outside the "exclusion radius", even
// if the AI is on top of the flee point, and he would just flee in-place.
//
static inline float zbiased_dist2(mxs_vector *pVec1, mxs_vector *pVec2)
{
   float fXDiff = pVec1->x-pVec2->x;
   float fYDiff = pVec1->y-pVec2->y;
   float fZDiff = pVec1->z-pVec2->z;

   if (fabs(fZDiff) < ZBIAS_THREASHHOLD)
      return fXDiff*fXDiff+fYDiff*fYDiff;
   else
   {
      fZDiff = fabs(fZDiff);
      return fXDiff*fXDiff+fYDiff*fYDiff+ZBIAS*fZDiff*fZDiff;
   }
}


#define kFleeSourceExclusionRadiusSq sq(20.0)

static int nState = -1;

//
// Chose next flee point object - either from FleeTo link (tried first), or from nearby flee points.
// Weakly state-driven.
// Provide Cleanup ability in case we get frame-limited, and don't get to go through all states.
//
// We assume that this will be called only be one "Pick" at a time.
static inline ObjID FeedTheBear(ObjID id, BOOL bCleanup = FALSE)
{
   static ILinkQuery * pQuery;
   static sPropertyObjIter iter;
   static BOOL bDidLinks;

   sLink link;

   if (nState == -1)
   {
      if (bCleanup) // already in reset state.
         return 0;

      pQuery = g_pAIFleeToLinks->Query(id, LINKOBJ_WILDCARD);
      nState = 0;
      if (pQuery->Done())
      {
         bDidLinks = FALSE;
         SafeRelease(pQuery);
         g_pAIFleePointProperty->IterStart(&iter);
         nState = 1; // Move onto just nearby patrol points
      }
   }

   switch (nState)
   {
      case 0: // Do FleeTo links
      {
         if (bCleanup)
         {
            SafeRelease(pQuery);
            nState = -1;
            return 0;
         }

         bDidLinks = TRUE;
         pQuery->Link(&link);
         pQuery->Next();
         if (pQuery->Done())
         {
            SafeRelease(pQuery);
            g_pAIFleePointProperty->IterStart(&iter);
            nState = 1; // Move onto just nearby patrol points
         }
         return link.dest;
      }
      break;

      case 1: // Nearby flee points
      {
         ObjID iterObj;

         if (bDidLinks)
         {
            nState = -1;
            return 0;
         }

         // For now, if we did look at links, then don't try nearby ones.
         if (bCleanup || bDidLinks ||!g_pAIFleePointProperty->IterNext(&iter, &iterObj))
         {
            nState = -1; // start over.
            g_pAIFleePointProperty->IterStop(&iter);
            return 0;
         }
         return iterObj;
      }
      break;

      default:
         return 0;
   }
}


static float SortedDists[MAX_FLEEPOINTS];

inline void cAIFlee::ResetFleePointList()
{
   m_nNumFleePoints = 0;
}


// Binary insertion:
inline void cAIFlee::InsertFleePoint(ObjID FleePointId, float fDist2, int nStartIx, int nEndIx)
{
   int nCurrentIx = (nStartIx+nEndIx) >> 1;

   if (nEndIx < nStartIx)
   {
      // memmove buffers the copy, unlike memcpy.
      if (nStartIx < m_nNumFleePoints)
      {
         memmove(m_SortedFleePoints+nStartIx+1, m_SortedFleePoints+nStartIx, sizeof(ObjID)*(m_nNumFleePoints-nStartIx));
         memmove(SortedDists+nStartIx+1, SortedDists+nStartIx, sizeof(float)*(m_nNumFleePoints-nStartIx));
      }
      SortedDists[nStartIx] = fDist2;
      m_SortedFleePoints[nStartIx] = FleePointId;
      return;
   }

   if (fDist2 > SortedDists[nCurrentIx])
      InsertFleePoint(FleePointId, fDist2, nCurrentIx+1, nEndIx);
   else
      InsertFleePoint(FleePointId, fDist2, nStartIx, nCurrentIx-1);
}



// This chooses flee points that are within our pathfinding "region", meaning the
// zones are the same, or connected. If there are no flee points, the we will path back to origin.
// When using flee points, we will make only a minimal effort to use the closest one; since we're fleeing, it's not
// that important that we do.
//
// Returns a pathable point that has the smallest 3d dist, but not necessarily the best path dist.
//

//@TBD: wsf: it would be nice if the AI didn't run toward player to get at flee point. To do this, we'd need
// to have some sort of callback into pathfinder that has the ability to reject cells. In this case, we'd reject
// based on angle between curloc and player, to curloc and first cell from curloc.

// From aipthfnd:
extern BOOL g_bPathfindControllerFailed;


ObjID cAIFlee::PickFleePoint(ObjID source)
{
   ObjID             FleePointId;
   mxs_vector        sourceLoc;
   mxs_vector        sourceLocRel; // source loc relative to us.
   mxs_vector        iterPoint;
   mxs_vector        iterPointRel; // flee point relative to us.
   mxs_vector        curLoc;
   tAIPathCellID     startCell;
   int               weight;
   int               i;
   float             fDist2;

   const int         nMaxPathfindsPerFrame = 3;
   int               nTotalPathfinds = 0;
   BOOL              bFrameLimitExceeded = FALSE;
   ObjID             best       = OBJ_NULL;
   float             bestDistSq = kFloatMax;
   tAIPathCellID     hintCell = 0;

   if (m_UnpathableRegions.Size() != g_AIPathDB.m_nNumFleeRegions)
      m_UnpathableRegions.SetSize(g_AIPathDB.m_nNumFleeRegions);

   if (!bFiguringDest)
   {
      if (m_pPath)
      {
         m_pPath->Release();
         m_pPath = 0;
      }
   }

   m_pPath = m_pExplicitPath;
   m_pExplicitPath = 0;
   if (m_pPath)
      m_pPath->AddRef();

   if (m_ExplicitDestId)
   {
      // Since two AI's might have been told to flee here in the same frame. First one to process
      // info wins.
      if (AIFleeIsCurrentDest(m_ExplicitDestId))
         return OBJ_NULL;
      // else, we're ok. Let's go here.
      return m_ExplicitDestId;
   }

   AssertMsg(!m_pPath, "Explicit Path defined for flee, but no object");

   GetObjLocation(source, &sourceLoc);
   GetObjLocation(GetID(), &curLoc);

   startCell = m_pAIState->GetPathCell();

   // Reset flee region info:
   // Build sorted list of points to test:
   if (!bFiguringDest)
   {
      for (i = 0; i < m_UnpathableRegions.Size(); i++)
         m_UnpathableRegions[i] = 0; // If region at index is unpathable, then it's set to non-zero

      ResetFleePointList();

      nState = -1;
      while (((FleePointId = FeedTheBear(GetID())) != 0) && (m_nNumFleePoints < MAX_FLEEPOINTS))
      {
         if (!OBJ_IS_CONCRETE(FleePointId))
            continue;

         if (g_pAINoFleeLinks->AnyLinks(GetID(), FleePointId))
            continue;

         GetObjLocation(FleePointId, &iterPoint);

         fDist2 = zbiased_dist2(&iterPoint, &curLoc);

         // Don't flee to a place too close to thing we're trying to flee from:
         if (zbiased_dist2(&iterPoint, &sourceLoc) < kFleeSourceExclusionRadiusSq)
            continue;

         // Don't flee to a place too close where we already are:
         if (fDist2 < kFleeSourceExclusionRadiusSq)
            continue;

         g_pAIFleePointProperty->Get(FleePointId, &weight);
         if (weight < 1)
            weight = 1;

         // Lame test to weight against flee points that are in general direction of source:
         mx_sub_vec(&iterPointRel, &iterPoint, &curLoc);
         mx_sub_vec(&sourceLocRel, &sourceLoc, &curLoc);
         if (mx_dot_vec(&sourceLocRel, &iterPointRel) > 0) // In direction of source, so weight against it.
            weight *= 0.2;

         fDist2 /= weight;

         InsertFleePoint(FleePointId, fDist2, 0, m_nNumFleePoints-1);
         m_nNumFleePoints++;
      }
   }

   // Then, go through and find first successful pathfind:

   // If these are labeled immediately pathable or unpathable, then they'll just loop right through. This is useful
   // for amortizing the cost of path calculation across frames.

   for (i = 0; i < m_nNumFleePoints; i++)
   {
      // See if we can path from our current zone to the fleepoint's zone.
      if (nTotalPathfinds > nMaxPathfindsPerFrame)
      {
         bFrameLimitExceeded = TRUE;
         break;
      }

      FleePointId = m_SortedFleePoints[i];

      GetObjLocation(FleePointId, &iterPoint);

      // pathdb stores flee points grouped by regions. That way, once we determine
      // a flee point is not pathable, then we can assume all flee points in that region are not
      // pathable either. This will save us from having to pathfind for all flee points.
      // At the same time, if we've already pathed to a flee point, then all flee points in that
      // region can be pathed to.

      if (IsInUnpathableRegion(FleePointId))
         continue;

      nTotalPathfinds++;

      // We do this, so the loco action gets path as it would have computed it itself:
      hintCell = 0;
      cAILocoAction::StrokeDest(&iterPoint, &hintCell, FleePointId);

      // If this failed because the controller didn't want to allow the AI to path, then DON'T mark
      // whole region.
      if ((m_pPath = AIPathfind(curLoc, startCell, iterPoint, 0,  kAIOK_Walk|m_pAIState->GetStressBits(), m_pAI->AccessPathfinder()->AccessControl())) == 0)
      {
         // If a door couldn't be opened, it may have led to this fleepoint, so don't mark whole
         // region unpathable. If we don't care about framerate, we could do a pathfind without
         // a controller to determine if we need to Mark this or not.
         if (!g_bPathfindControllerFailed)
            MarkUnpathable(FleePointId);
         continue;
      }
      else
         m_pPath->AddRef();

      // we found one we can path to. We're done.
      break;
   }

   bFiguringDest = bFrameLimitExceeded;

#if 0 // @TBD:
   // We found no flee points. Start a Dynamic Flee.
   if (!bFiguringDest && !m_pPath)
   {
      AIWatch(Flow, GetID(), "Found no flee points. Attempting dynamic Flee");

   }
#endif


   if (!bFiguringDest)
      AIWatchTrue(!m_pPath, Flow, GetID(), "Failed to locate a valid flee point");
   else
      return OBJ_NULL;

   return FleePointId;
}

///////////////////////////////////////

void cAIFlee::SetFleeSource(ObjID objId)
{
   bWasDamage = FALSE; // Clear this. Only OnDamage sets it.
   nFleeSource = objId; // So we can try to flee again if we lose track of it, but still are at high alert.

   LinkID linkId = g_pAIFleeSourceLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   m_ExplicitDestId = 0;
   bFiguringDest = FALSE;

   if (m_pExplicitPath)
   {
      m_pExplicitPath->Release();
      m_pExplicitPath = 0;
   }

   if (linkId)
   {
      sLink  link;

      g_pAIFleeSourceLinks->Get(linkId, &link);
      if (link.dest == objId)
         return;
      g_pAIFleeDestLinks->Remove(linkId);
   }

   g_pAIFleeSourceLinks->Add(GetID(), objId);
}

///////////////////////////////////////

const sAIFleeDest * cAIFlee::GetFleeDest()
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   sLink  link;

   if (!linkId)
      return NULL;

   g_pAIFleeDestLinks->Get(linkId, &link);

   sAIFleeDest * pResult = (sAIFleeDest * )g_pAIFleeDestLinks->GetData(linkId);
   pResult->object = link.dest;

   return pResult;
}

///////////////////////////////////////

void cAIFlee::SetFleeDest(ObjID dest)
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (linkId)
   {
      // If we previously selected a point, it couldn't have been a designer
      // "no flee" link. So, we renenable the point as valid
      sLink link;
      g_pAIFleeDestLinks->Get(linkId, &link);
      LinkID noFleeLinkId = g_pAINoFleeLinks->GetSingleLink(GetID(), link.dest);
      if (noFleeLinkId)
         g_pAINoFleeLinks->Remove(noFleeLinkId);
   }

   if (!dest)
   {
      if (linkId)
         g_pAIFleeDestLinks->Remove(linkId);
      return;
   }

   if (linkId)
      g_pAIFleeDestLinks->Remove(linkId);

   sAIFleeDest fleeDest;

   fleeDest.object     = dest;
   fleeDest.fReached   = FALSE;
   fleeDest.expiration = 0;

   g_pAIFleeDestLinks->AddFull(GetID(), dest, (void *)&fleeDest);
}

///////////////////////////////////////

void cAIFlee::FlagFleeDestReached()
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   sLink  link;

   if (!linkId)
   {
#ifndef SHIP
      mprintf("Flee destination link for AI %d is missing! [%s@%d]\n", GetID(), __FILE__, __LINE__);
#endif
      return;
   }

   g_pAIFleeDestLinks->Get(linkId, &link);

   sAIFleeDest * pDest = (sAIFleeDest * )g_pAIFleeDestLinks->GetData(linkId);
   pDest->fReached = TRUE;
   g_pAIFleeDestLinks->SetData(linkId, pDest);

   g_pAINoFleeLinks->Add(GetID(), link.dest);

   // We're at our dest. Force an inform, if we're of a mind to do so.
   // If this AI has no inform property, then this'll just get ignored.
   // If inform ability should fire, then it informs immediately anyway, and this bit will get cleared.
   g_pAIForceImmediateInform->Set(GetID(), TRUE);
}

///////////////////////////////////////

BOOL cAIFlee::GetFleeDestReached()
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (!linkId)
   {
#ifndef SHIP
      mprintf("Flee destination link for AI %d is missing! [%s@%d]\n", GetID(), __FILE__, __LINE__);
#endif
      return TRUE;
   }

   sAIFleeDest * pDest = (sAIFleeDest * )g_pAIFleeDestLinks->GetData(linkId);
   return pDest->fReached;
}

///////////////////////////////////////

unsigned cAIFlee::GetFleeDestExpiration()
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (!linkId)
   {
#ifndef SHIP
      mprintf("Flee destination link for AI %d is missing! [%s@%d]\n", GetID(), __FILE__, __LINE__);
#endif
      return 0;
   }

   sAIFleeDest * pDest = (sAIFleeDest * )g_pAIFleeDestLinks->GetData(linkId);
   return pDest->expiration;
}

///////////////////////////////////////

void cAIFlee::SetFleeDestExpiration(unsigned time)
{
   LinkID linkId = g_pAIFleeDestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (!linkId)
   {
#ifndef SHIP
      mprintf("Flee destination link for AI %d is missing! [%s@%d]\n", GetID(), __FILE__, __LINE__);
#endif
      return;
   }

   sAIFleeDest * pDest = (sAIFleeDest * )g_pAIFleeDestLinks->GetData(linkId);
   pDest->expiration = time;
   g_pAIFleeDestLinks->SetData(linkId, pDest);
}

///////////////////////////////////////

void cAIFlee::StopFlee()
{
   bDoingInvestigate = FALSE;
   LinkDestroyMany(GetID(), LINKOBJ_WILDCARD, g_pAIFleeSourceLinks->GetID());
   LinkDestroyMany(GetID(), LINKOBJ_WILDCARD, g_pAIFleeDestLinks->GetID());
   LinkDestroyMany(GetID(), LINKOBJ_WILDCARD, g_pAINoFleeLinks->GetID());
   SignalGoal();
}

///////////////////////////////////////////////////////////////////////////////
