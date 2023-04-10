/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicombat.cpp,v 1.45 2000/02/11 18:28:09 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>
#include <config.h>
#include <objedit.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <playrobj.h>
#include <relation.h>

#include <dmgbase.h>
#include <mvrflags.h>
#include <weapcb.h>

#include <aiapibhv.h>
#include <aiapinet.h>
#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiactloc.h>
#include <aialert.h>
#include <aiaware.h>
#include <aicombat.h>
#include <aigoal.h>
#include <aiprabil.h>
#include <aiprcore.h>
#include <aisndtyp.h>
#include <aisubcb.h>
#include <aiteams.h>
#include <aitagtyp.h>
#include <aiutils.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <propbase.h>
#include <propert_.h>
#include <prophash.h>

#include <simtime.h>

#include <iobjsys.h>
#include <traitman.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static BOOL g_NoCombatRehosting=FALSE; // Set when defined config var "ai_no_combat_rehosting"

///////////////////////////////////////////////////////////////////////////////
//
// Combat timing property
//

#define kAINoContactCreatePeriod 2500
#define kAINoContactDropPeriod 3500

#define kAILostContactMinDelay 6000
#define kAILostContactMaxDelay 16000

struct sAICombatTiming
{
   tSimTime noContactCreatePeriod;
   tSimTime noContactDropPeriod;
   tSimTime lostContactMinDelay;
   tSimTime lostContactMaxDelay;
};

///////////////////////////////////////

sAICombatTiming _g_AIDefaultCombatTiming =
{
   kAINoContactCreatePeriod,
   kAINoContactDropPeriod,
   kAILostContactMinDelay,
   kAILostContactMaxDelay,
};

///////////////////////////////////////

static sFieldDesc _g_AICombatTimingFieldDesc[] =
{
   { "No contact create period",      kFieldTypeInt,  FieldLocation(sAICombatTiming, noContactCreatePeriod) }, \
   { "No contact drop period",        kFieldTypeInt,  FieldLocation(sAICombatTiming, noContactDropPeriod) }, \
   { "Lost contact min delay",        kFieldTypeInt,  FieldLocation(sAICombatTiming, lostContactMinDelay) }, \
   { "Lost contact max delay",        kFieldTypeInt,  FieldLocation(sAICombatTiming, lostContactMaxDelay) }, \
};

static sStructDesc _g_AICombatTimingStructDesc = StructDescBuild(sAICombatTiming, kStructFlagNone, _g_AICombatTimingFieldDesc);

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAICombatTimingProperty
DECLARE_PROPERTY_INTERFACE(IAICombatTimingProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAICombatTiming *);
};

///////////////////

static sPropertyDesc _g_CombatTimingPropertyDesc =
{
   "AI_CbtTiming",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Combat: Timing" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAICombatTimingProperty, &IID_IAICombatTimingProperty, sAICombatTiming *, cHashPropertyStore< cClassDataOps<sAICombatTiming> > > cAICombatTimingPropertyBase;

class cAICombatTimingProperty : public cAICombatTimingPropertyBase
{
public:
   cAICombatTimingProperty()
      : cAICombatTimingPropertyBase(&_g_CombatTimingPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAICombatTiming);
};

///////////////////

static IAICombatTimingProperty * g_pAICombatTimingProperty;

#define AIGetCombatTiming(obj) AIGetProperty(g_pAICombatTimingProperty, (obj), (sAICombatTiming *)&_g_AIDefaultCombatTiming)

///////////////////////////////////////////////////////////////////////////////

IRelation * g_pAIAttackLinks;
IRelation * g_pAICurrentTargetLinks;

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitCombatAbility(IAIManager *)
{                                            // for now, since AIAttack is networked
   sRelationDesc     combatDesc     = { "AIAttack", kRelationNetProxyChangeable, 0, 0 };
   sRelationDataDesc combatDataDesc = { "eAIPriority", sizeof(eAIPriority), kRelationDataAutoCreate };
   sRelationDesc     curTargDesc    = { "AITarget", 0, 0, 0 };
   sRelationDataDesc noDataDesc     = { "None", 0 };

   g_pAIAttackLinks        = CreateStandardRelation(&combatDesc, &combatDataDesc, (kQCaseSetSourceKnown | kQCaseSetDestKnown));
   g_pAICurrentTargetLinks = CreateStandardRelation(&curTargDesc, &noDataDesc, (kQCaseSetSourceKnown));

   g_pAICombatTimingProperty = new cAICombatTimingProperty;
   StructDescRegister(&_g_AICombatTimingStructDesc);

   g_NoCombatRehosting = config_is_defined("ai_no_combat_rehosting");

   return TRUE;
}

///////////////////////////////////////

BOOL AITermCombatAbility()
{
   SafeRelease(g_pAICurrentTargetLinks);
   SafeRelease(g_pAIAttackLinks);
   SafeRelease(g_pAICombatTimingProperty);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombat
//

cAICombat::cAICombat()
 : m_TargetRefreshTimer(kAIT_2Hz),
   m_ReactChargeTimer(20000, 90000),
   m_LostContactTimer(kAILostContactMinDelay, kAILostContactMaxDelay),
   m_targetAlertnessThreshold(kAIAL_High),
   m_lastTargetSwitch(GetSimTime())
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAICombat::GetName()
{
   return "Combat core";
}

///////////////////////////////////////
//
// Init()
//
// Grab sub-components. cAICombat does not hold a reference to the
// contained sub abilities.
//

STDMETHODIMP_(void) cAICombat::Init()
{
   cAIAbility::Init();

   m_LostContactTimer.Set(AIGetCombatTiming(GetID())->lostContactMinDelay, AIGetCombatTiming(GetID())->lostContactMaxDelay);
   m_LostContactTimer.Delay(kAIT_Infinite);
   m_LostContactWith = OBJ_NULL;

   SetNotifications(kAICN_GoalChange |
                    kAICN_ActionProgress |
                    kAICN_Alertness |
                    kAICN_Property |
                    kAICN_Damage |
                    kAICN_Weapon |
                    kAICN_Sound);

   AddMotionFlagListener(m_pAIState->GetID(), MF_FIRE_RELEASE, NULL);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::Term()
{
   RemoveMotionFlagListener(m_pAIState->GetID(), MF_FIRE_RELEASE);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAICombat::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Combat, 0, 1, pTagFile))
   {
      BaseAbilitySave(pTagFile);
      m_TargetRefreshTimer.Save(pTagFile);
      m_ReactChargeTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAICombat::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Combat, 0, 1, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      m_TargetRefreshTimer.Load(pTagFile);
      m_ReactChargeTimer.Load(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAIAbility::OnGoalChange(pPrevious, pGoal);

   if (IsGainingControl(pPrevious, pGoal) || (IsOwn(pGoal) && pPrevious != pGoal))
   {
      AutoAppIPtr(AINetServices);
      pAINetServices->TransferAI(GetID(), GetTarget());
   }
   else if (IsLosingControl(pPrevious, pGoal))
      SetTarget(OBJ_NULL);
}

///////////////////////////////////////

DECLARE_TIMER(cAICombat_OnActionProgress, Average);

STDMETHODIMP_(void) cAICombat::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAICombat_OnActionProgress);

   // Refresh the target info...
   if (m_TargetRefreshTimer.Expired() && pAction->GetResult() > kAIR_NoResult)
   {
      // Look for new targets
      ObjID oldTarget = GetTarget();
      if (SeekCombat())
      {
         // We want to be in Combat mode and in control
         if (m_pAIState->GetMode() < kAIM_Combat)
            SignalMode();                        // Mode will signal goal
         else if (!InControl() || GetTarget() != oldTarget)
            SignalGoal();
      }
      // Otherwise, we want to bail our of Combat
      else
      {
         if (m_pAIState->GetMode() == kAIM_Combat)
            SignalMode();                        // Mode will signal goal
      }

      m_TargetRefreshTimer.Reset();
   }

   if (IsOwn(pAction))
   {
      if (m_pAIState->GetAlertness() == kAIAL_High && m_pAI->AccessSoundEnactor())
      {
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_AlertThreeBroadcast);
      }

      // If we just completed our own goal...
      if (!pAction->InProgress())
      {
         // Signal for more
         SignalAction();
      }
   }

   CheckLostContact();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::OnAlertness(ObjID source,
                                           eAIAwareLevel previous,
                                           eAIAwareLevel current,
                                           const sAIAlertness * pRaw)
{
   if (previous == kAIAL_High)
      CheckLostContact();

   // If we're changing alertness, verify that we should or should
   // not be in Combat mode
   SeekCombat();
   SignalMode();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::OnDamage(const sDamageMsg *pMsg, ObjID realCulpritID)
{
   if (pMsg->kind != kDamageMsgDamage)
      return;

   if (m_flags & kWasDamaged)
      return;

   m_flags |= kWasDamaged;

   if (!InControl())
   {
      SeekCombat();
      SignalMode();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::OnWeapon(eWeaponEvent ev, ObjID victim, ObjID culprit)
{
   if (m_flags & kWasThreatened)
      return;

   if (culprit != GetID() && AITeamCompare(AIGetTeam(culprit), AIGetTeam(GetID())) == kAI_Opponents)
      m_flags |= kWasThreatened;

   if ((m_flags & kWasThreatened) && !InControl())
   {
      SeekCombat();
      SignalMode();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::OnProperty(IProperty * pProperty, const sPropertyListenMsg *)
{
   if (pProperty == g_pAINonHostilityProperty)
   {
      // If we're changing hostility, this will verify Combat mode
      SeekCombat();
      SignalMode();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombat::OnSound(const sSoundInfo *, const sAISoundType * pType)
{
   if (pType->type == kAIST_Combat)
   {
      if (m_flags & kWasThreatened)
         return;

      m_flags |= kWasThreatened;

      if (!InControl())
      {
         SeekCombat();
         SignalMode();
      }
   }
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAICombat::UpdateGoal(cAIGoal * pGoal)
{
   return kAIR_NoResult;
}

///////////////////////////////////////

STDMETHODIMP cAICombat::SuggestMode(sAIModeSuggestion * pModeSuggestion)
{
   // If we have a target...
   if (GetTarget())
   {
      // We'd like to be in Combat mode
      pModeSuggestion->mode     = kAIM_Combat;
      pModeSuggestion->priority = kAIP_High;
   }
   else
   {
      // Otherwise, normal
      pModeSuggestion->mode     = kAIM_Normal;
      pModeSuggestion->priority = kAIP_Normal;
   }

   SignalGoal();

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAICombat::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   ObjID target = GetTarget();

   if (!target)
   {
      if (InControl())
      {
         if (IsAPlayer(pPrevious->object))
         {
            m_LostContactTimer.Reset();
            m_LostContactWith = pPrevious->object;
         }
         SignalMode();
      }

      *ppNew = NULL;
      return S_OK;
   }

   cAIAttackGoal * pGoal = new cAIAttackGoal(this);
   pGoal->object = target;
   pGoal->priority = kAIP_High;

   pGoal->LocFromObj();

   SignalAction();
   *ppNew = pGoal;

   return S_OK;
}

///////////////////////////////////////
//
//
//

STDMETHODIMP cAICombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAILocoAction * pAction = CreateLocoAction();
   pAction->SetFromGoal((cAIGotoGoal *)pGoal);
   pNew->Append(pAction);
   return S_OK;
}

///////////////////////////////////////

ObjID cAICombat::GetTarget()
{
   LinkID linkId = g_pAICurrentTargetLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   sLink  link;

   if (!linkId)
      return OBJ_NULL;

   g_pAICurrentTargetLinks->Get(linkId, &link);

   return link.dest;
}

///////////////////////////////////////

const mxs_vector & cAICombat::GetTargetLoc()
{
   static mxs_vector result;
   ObjID target = GetTarget();
   AssertMsg(target, "Cannot get target location without a target");
   const sAIAwareness * pAwareness  = m_pAI->GetAwareness(target);
   if (pAwareness->flags & kAIAF_CanRaycast)
      GetObjLocation(target, &result);
   else if (pAwareness->ValidLastPos())
      result = pAwareness->lastPos;
   else
      result = *m_pAIState->GetLocation();
   return result;
}

///////////////////////////////////////

ObjID cAICombat::SeekCombat()
{
   m_TargetRefreshTimer.Reset();

   // Freshen info on existing targets
   UpdateTargets();

   // And look for new targets
   return TargetScan();
}

///////////////////////////////////////

// See if we need to play the LostContact concept.
void cAICombat::CheckLostContact()
{
   // If I lost contact with a player a while ago, and no other AI's have any contact with that
   // player, then player the LostContact concept.
   if (m_LostContactTimer.Expired())
   {
      Assert_(m_LostContactWith != OBJ_NULL);
      if (!g_pAIAttackLinks->AnyLinks(LINKOBJ_WILDCARD, m_LostContactWith))
          if (m_pAI->AccessSoundEnactor())
          {
             m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_LostContact);
          }
      m_LostContactTimer.Delay(kAIT_Infinite);
      m_LostContactWith = OBJ_NULL;
   }
}

///////////////////////////////////////
//
// Is this target either to acquire or retain?
//
BOOL cAICombat::IsValidTarget(ObjID object)
{
   return (object != GetID() && IsHostileToward(object) && AIGetMode(object) != kAIM_Dead);
}

///////////////////////////////////////
//
// Is this object valid to acquire as a target?
// Checks awareness type, level, existing attack link
//
BOOL cAICombat::IsAcquirableTarget(const sAIAwareness& awareness)
{
   ObjID object = awareness.object;
   eAIAwareLevel thresh = (m_pAIState->GetAlertness() >= m_targetAlertnessThreshold) ? kAIAL_Low : kAIAL_High;

   return (!g_pAIAttackLinks->GetSingleLink(GetID(), object) &&
      (awareness.flags & kAIAF_FirstHand) &&
      (awareness.flags & kAIAF_Seen) &&
      awareness.TimeSinceContact() <= AIGetCombatTiming(GetID())->noContactCreatePeriod &&
      awareness.level >= thresh);
}

///////////////////////////////////////
//
// How good a target is the object?
//

#define kLessImportantDist sq(30.0)
#define kMostImportantDist sq(15.0)

eAIPriority cAICombat::TargetPriority(ObjID object)
{
   eAIPriority priority;
   float distSq = m_pAIState->DistSq(*GetObjLocation(object));

   if (distSq > kLessImportantDist)
      priority = kAIP_VeryLow;
   else if (distSq < kMostImportantDist)
      priority = kAIP_VeryHigh;
   else
      priority = kAIP_Normal;

   // Player bonus
   if (IsAPlayer(object))
      priority = (eAIPriority)((int)priority + 1);

   return priority;
}

///////////////////////////////////////
//
// Scan for new targets
//

ObjID cAICombat::TargetScan()
{
   if (m_pAIState->GetAlertness() > kAIAL_Lowest)
   {
      cAIAwareArray awarenesses;
      eAIPriority priority;

      m_pAI->AccessSenses()->GetAllAwareness(&awarenesses);
      for (int i = 0; i < awarenesses.Size(); i++)
      {
         ObjID object = awarenesses[i]->object;
         if (IsValidTarget(object) && IsAcquirableTarget(*awarenesses[i]) &&
            ((priority = TargetPriority(object))>kAIP_None))
            g_pAIAttackLinks->AddFull(GetID(), object, &priority);
      }
   }

   ObjID target = SelectTarget();

   SetTarget(target);

   return target;
}

///////////////////////////////////////

struct sTargScanInfo
{
   ObjID       id;
   float       distSq;
   eAIPriority priority;
};

#define kCurTargBonus sq(15.0)
#define kRaycastBonus sq(10.0)
#define kWayWayClose  sq(5.0)
#define kCloseBonus   sq(25.0)
#define kMaxCurTime   10000
#define kTimeTooShort  1000

static int TargetSort(const sTargScanInfo * pLeft, const sTargScanInfo * pRight)
{
   // note reversed, doing largest to smallest
   // dc - i changed this to smallest to largest, but i may be on crack
   int result;

   result = pRight->priority - pLeft->priority;

   if (!result)
   {
      float diff = pRight->distSq - pLeft->distSq;

      result = (diff < 0) ?  1 :
                  (diff > 0) ? -1 : 0;
   }
   return result;
}


ObjID cAICombat::SelectTarget()
{
   if (m_pAIState->GetAlertness() < kAIAL_Moderate)
      return OBJ_NULL;

   cDynArray_<sTargScanInfo, 16> targets;
   ObjID                         current = GetTarget();
   int                           time_this_target=GetSimTime()-m_lastTargetSwitch;

   ILinkQuery * pLinkQuery;
   sLink        link;
   IAISenses *  pSenses = m_pAI->AccessSenses();

   pLinkQuery = g_pAIAttackLinks->Query(GetID(), LINKOBJ_WILDCARD);

   // @TBD: really, if we only have one, we just want to leave now...

   while (!pLinkQuery->Done())
   {
      pLinkQuery->Link(&link);

      if (time_this_target<kTimeTooShort && link.dest == current)
      {
         SafeRelease(pLinkQuery);
         return current;
      }

      int i = targets.Grow();

      targets[i].id       = link.dest;
      targets[i].priority = *((eAIPriority *)pLinkQuery->Data());
      targets[i].distSq   = m_pAIState->DistSq(*GetObjLocation(link.dest));
      // maybe make these all multiplicitive?

      if (targets[i].distSq<kWayWayClose)
         targets[i].distSq -= kCloseBonus;

      if (link.dest == current)
         if (time_this_target<kMaxCurTime)  // ??? ummm - quadratic is screwing this
            targets[i].distSq -= (kCurTargBonus*(1-(time_this_target/kMaxCurTime)));

      if (pSenses->GetAwareness(link.dest)->flags & kAIAF_CanRaycast)
         targets[i].distSq -= kRaycastBonus;

      pLinkQuery->Next();
   }

   SafeRelease(pLinkQuery);

   if (!targets.Size())
      return OBJ_NULL;

   targets.Sort(TargetSort);

   return targets[0].id;
}

///////////////////////////////////////

void cAICombat::SetTarget(ObjID target)
{
   LinkID linkId = g_pAICurrentTargetLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (linkId)
   {
      sLink  link;
      g_pAICurrentTargetLinks->Get(linkId, &link);
      if (link.dest == target)
         return;
      g_pAICurrentTargetLinks->Remove(linkId);
   }

   if (target)
      g_pAICurrentTargetLinks->Add(GetID(), target);
}

///////////////////////////////////////
//
// Should we retain this target
// Checks awareness, distance & LOS
//

#define kTooCloseToLoseDist sq(16.0)

BOOL cAICombat::IsRetainableTarget(const sAIAwareness& awareness)
{
   ObjID object         = awareness.object;
   BOOL  awareOfTarget  = (awareness.level>kAIAL_Lowest);
   float distSq         = m_pAIState->DistSq(*GetObjLocation(awareness.object));
   BOOL  tooCloseToLose = ((awareness.flags & kAIAF_CanRaycast) && (distSq < kTooCloseToLoseDist));

   if (!tooCloseToLose && (!awareOfTarget || AIGetTime() - awareness.lastContact > AIGetCombatTiming(GetID())->noContactDropPeriod))
      return FALSE;

   return TRUE;
}

///////////////////////////////////////
//
// Check up on attack links
//

void cAICombat::UpdateTargets()
{
   ILinkQuery *         pLinkQuery;
   sLink                link;
   const sAIAwareness * pAwareness;
   eAIPriority priority;

   pLinkQuery = g_pAIAttackLinks->Query(GetID(), LINKOBJ_WILDCARD);

   while (!pLinkQuery->Done())
   {
      pLinkQuery->Link(&link);
      pAwareness = m_pAI->GetAwareness(link.dest);

      // If the AI has not sensed the player after a while, we kill the link
      if (!IsValidTarget(pAwareness->object) || !IsRetainableTarget(*pAwareness) ||
         ((priority = TargetPriority(pAwareness->object))==kAIP_None))
         g_pAIAttackLinks->Remove(pLinkQuery->ID());
      else  // really, want to see data change, not just set/send every time
         if (priority!=(*((int *)pLinkQuery->Data())))
            g_pAIAttackLinks->SetData(pLinkQuery->ID(), &priority);

      pLinkQuery->Next();
   }
   SafeRelease(pLinkQuery);
}

///////////////////////////////////////

enum eAINonHostilityEnum
{
   kAINH_Never,
   kAINH_ToPlayer,
   kAINH_ToPlayerUntilDmg,
   kAINH_ToPlayerUntilThreat,
   kAINH_UntilDmg,
   kAINH_UntilThreat,
   kAINH_Always,

   kAINH_Num,

   kAINH_TypeMax = 0xffffffff
};

typedef unsigned eAINonHostility;

static BOOL DoesProvokeBloodlust(ObjID us, ObjID them)
{
   BOOL bVal = FALSE;

   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);

   if (!g_pAIIgnoresCamerasProperty || !g_pAIIgnoresCamerasProperty->Get(us, &bVal) || !bVal)
      return TRUE;

   ObjID Camera = pObjectSystem->GetObjectNamed("Cameras");
   if (Camera == OBJ_NULL)
      return TRUE;

   return !pTraitManager->ObjHasDonor(them, Camera);
}


BOOL cAICombat::IsHostileToward(ObjID obj)
{
   // First, check the team
   if ((AITeamCompare(AIGetTeam(obj), AIGetTeam(GetID())) != kAI_Opponents) || !DoesProvokeBloodlust(GetID(), obj))
      return FALSE;

   // Then check the non-hostility property
   eAINonHostility nonHostility = AIGetNonHostility(GetID());
   BOOL result = TRUE;

   switch (nonHostility)
   {
      case kAINH_Never:
         break;

      case kAINH_ToPlayer:
         result = (!IsAPlayer(obj));
         break;

      case kAINH_ToPlayerUntilDmg:
         result = (!IsAPlayer(obj) || (m_flags & kWasDamaged));
         break;

      case kAINH_ToPlayerUntilThreat:
         result = (IsAPlayer(obj) || (m_flags & (kWasDamaged | kWasThreatened)));
         break;

      case kAINH_UntilDmg:
         result = !!(m_flags & kWasDamaged);
         break;

      case kAINH_UntilThreat:
         result = !!(m_flags & (kWasDamaged | kWasThreatened));
         break;

      case kAINH_Always:
         result = FALSE;
         break;

      default:
         CriticalMsg("Bad non-hostility value");
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
