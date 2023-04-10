/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiinvest.cpp,v 1.21 2000/02/11 18:27:49 bfarquha Exp $
//
// @TBD (toml 05-22-98): parameters in here need to be propertized
// @TBD (toml 05-23-98): low level investigate behavior (action series) should be moved into cAIInvestAction
// @TBD (toml 05-23-98): we probably need a sense edge notification
// @TBD (toml 05-23-98): deal with case where this ability loses goal arbitration
// @TBD (toml 05-23-98): consider providing previous goal on goal change for better edge detection
// @Note (toml 05-23-98): the following code assumes always only one real action,
// which is true of this ability right now but might change depending on
// how sound gets reworked
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <linkbase.h>
#include <playrobj.h>
#include <relation.h>

#include <aiactinv.h>
#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aiapibhv.h>
#include <aidfndpt.h>
#include <aigests.h>
#include <aigoal.h>
#include <aiinvest.h>
#include <aiinvtyp.h>
#include <aiprabil.h>
#include <aisndtyp.h>
#include <aitagtyp.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

// @TBD (toml 05-23-98): tune, config
int g_AIInvestigateMinSenseResync = 50;
int g_AITimeBreakInvestigation = 30 * 1000;

#define kMinSenseResyncDelay   500
#define kMaxSenseResyncDelay  1000
#define kMinResyncDelay       5000
#define kMaxResyncDelay      18000

IRelation * g_pAIInvestLinks;

///////////////////////////////////////////////////////////////////////////////

void AIInitInvestigateAbility()
{
   sRelationDesc     investDesc     = { "AIInvest", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc noDataDesc     = { "None", 0 };

   g_pAIInvestLinks = CreateStandardRelation(&investDesc, &noDataDesc, (kQCaseSetSourceKnown));
}

///////////////////////////////////////

void AITermInvestigateAbility()
{
   SafeRelease(g_pAIInvestLinks);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIInvestigate
//

cAIInvestigate::cAIInvestigate()
 : m_MaxSenseResyncTimer(kMinSenseResyncDelay, kMaxSenseResyncDelay),
   m_MaxResyncTimer     (kMinResyncDelay, kMaxResyncDelay)
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIInvestigate::GetName()
{
   return "Investigation ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInvestigate::Init()
{
   cAINonCombatAbility::Init();

   SetNotifications(kAICN_Alertness | kAICN_Property | kAICN_Defend);
}

///////////////////////////////////////

#define kInvestVer 2

STDMETHODIMP_(BOOL) cAIInvestigate::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Investigate, 0, kInvestVer, pTagFile))
   {
      BaseAbilitySave(pTagFile);

// @TBD (toml 10-05-98): action
      m_MaxSenseResyncTimer.Save(pTagFile);
      m_MaxResyncTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIInvestigate::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Investigate, 0, kInvestVer, pTagFile))
   {
      BaseAbilityLoad(pTagFile);

      m_MaxSenseResyncTimer.Save(pTagFile);
      m_MaxResyncTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInvestigate::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAINonCombatAbility::OnGoalChange(pPrevious, pGoal);
   if (IsOwn(pGoal))
   {
      m_MaxSenseResyncTimer.Reset();
      m_MaxResyncTimer.Reset();

   }
   else
      ClearNotifications(kAICN_GoalChange);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInvestigate::OnAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   cAINonCombatAbility::OnAlertness(source, previous, current, pRaw);
   if (current >= kAIAL_Moderate)
   {
      SetAlertnessSource(source);
      SetNotifications(kAICN_ActionProgress);
   }
   else
   {
      SetAlertnessSource(OBJ_NULL);
      ClearNotifications(kAICN_ActionProgress);
   }
   SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInvestigate::OnProperty(IProperty * pProperty, const sPropertyListenMsg *)
{
   if (pProperty == g_pAIInvestKindProperty)
   {
      if (GetAlertnessSource() && AIGetInvestKind(GetID()) == kAIIK_Never)
         SetAlertnessSource(OBJ_NULL);
      SignalGoal();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInvestigate::OnDefend(const sAIDefendPoint * pPoint)
{
   SignalGoal();
}

///////////////////////////////////////

DECLARE_TIMER(cAIInvestigate_OnActionProgress, Average);

STDMETHODIMP_(void) cAIInvestigate::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIInvestigate_OnActionProgress);

   BOOL fSignal  = FALSE;

   if (InControl())
   {
      cAIInvestAction * pInvestAction = (cAIInvestAction *)pAction;
      // If it's done (reached investigation point) ...
      if (!pInvestAction->InProgress())
      {
         // Signal for a new goal
         fSignal = TRUE;
      }
      else if (pInvestAction->IsScanning())
         return;

      fSignal = m_MaxResyncTimer.Expired();
   }

   if (m_pAI->AccessSenses() && m_pAIState->GetAlertness() >= kAIAL_Moderate && m_MaxSenseResyncTimer.Expired())
   {
      m_MaxSenseResyncTimer.Reset();

      const sAIAwareness * pAwareness = m_pAI->AccessSenses()->GetHighAwareness(kAISGH_OpponentsOnly, g_AITimeBreakInvestigation);

      if (pAwareness)
      {
         SetAlertnessSource(pAwareness->object);
      }
      else
         SetAlertnessSource(OBJ_NULL);

      if (!fSignal)
         fSignal = (GetAlertnessSource() || InControl());
   }

    if (fSignal)
       SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIInvestigate::UpdateGoal(cAIGoal * pGoal)
{
   // We are never really done...
   return kAIR_NoResult;
}

///////////////////////////////////////

STDMETHODIMP cAIInvestigate::SuggestMode(sAIModeSuggestion * pModeSuggestion)
{
   pModeSuggestion->mode     = kAIM_Normal;
   pModeSuggestion->priority = kAIP_VeryLow;
   return S_OK;
}

///////////////////////////////////////
// @TBD (toml 05-23-98):

STDMETHODIMP cAIInvestigate::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   Assert_(GetNotifications() & kAICN_BeginFrame);
   //
   // If the alertness source is not from an object, assume it is not from a
   // direct disturbance.
   //
   if (!GetAlertnessSource() ||
       AIGetInvestKind(GetID()) == kAIIK_Never)
   {
      *ppNew = NULL;
      return S_OK;
   }

   //
   // If we're no longer aware of the source, or enough time has passed since
   // actual sensing, don't investigate
   //
   const sAIAwareness * pAwareness = m_pAI->GetAwareness(GetAlertnessSource());

   if (!pAwareness->ValidLastPos() || pAwareness->TimeSinceContact() > g_AITimeBreakInvestigation)
   {
      *ppNew = NULL;
      SetAlertnessSource(OBJ_NULL);
      return S_OK;
   }

   //
   // For now, only investigate if ai or target is within defend range
   //
   const sAIDefendPoint * pDefend = AIGetDefendPoint(GetID());

   if (pDefend && !pDefend->fInRange && !pDefend->InRange(pAwareness->lastPos))
   {
      *ppNew = NULL;
      SetAlertnessSource(OBJ_NULL);
      return S_OK;
   }

   // @TBD (toml 05-23-98): if not sense, and distance to player is great, bail

   //
   // Investigate!
   //

   *ppNew = new cAIInvestigateGoal(this);

   (*ppNew)->priority  = kAIP_Low;
   (*ppNew)->object    = GetAlertnessSource();
   (*ppNew)->location  = pAwareness->lastPos;
   (*ppNew)->ownerData = !!pDefend;

   SetNotifications(kAICN_GoalChange);

   SignalAction();

   return S_OK;
}

///////////////////////////////////////

float g_InvestNoFreshenRange = 25.0;
float g_InvestSpeed = 7.0;

STDMETHODIMP cAIInvestigate::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   IAISenses * pSenses = m_pAI->AccessSenses();
   ObjID source = GetAlertnessSource();
   BOOL freshen = (pSenses && IsAPlayer(source));

   if (freshen)
      pSenses->KeepFresh(source, 0);

   if (source == OBJ_NULL)
   {
      SignalGoal();
      return S_FALSE;
   }

   cAIInvestAction * pAction;
   if (!previous.Size())
   {
      pAction = CreateInvestAction();
      pAction->SetStyle(kAIIS_InitScan | kAIIS_VarSpeed);
   }
   else
   {
      pAction = (cAIInvestAction *)previous[0];
      pAction->AddRef();
   }

   if (memcmp(&pAction->GetDest(), &pGoal->location, sizeof(mxs_vector)) != 0)
      pAction->Set(pGoal->location, source);

   if (freshen)
   {
      float distKeepFreshSense = pAction->ComputePathDist() - g_InvestNoFreshenRange;

      if (distKeepFreshSense > 0.0)
         pSenses->KeepFresh(source, (distKeepFreshSense / g_InvestSpeed) * 1000.0);
   }

   pNew->Append(pAction);

   return S_OK;
}

///////////////////////////////////////

void cAIInvestigate::BroadcastAlertness()
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

   if (m_pAI->AccessSoundEnactor() && concept != kAISC_CoreNum)
   {
      cTagSet investigateTags("Investigate true");
      m_pAI->AccessSoundEnactor()->RequestConcept(concept, &investigateTags);
   }
}

///////////////////////////////////////

ObjID cAIInvestigate::GetAlertnessSource()
{
   LinkID linkId = g_pAIInvestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);
   sLink  link;

   if (!linkId)
      return OBJ_NULL;

   g_pAIInvestLinks->Get(linkId, &link);

   return link.dest;
}

///////////////////////////////////////

void cAIInvestigate::SetAlertnessSource(ObjID to)
{
   LinkID linkId = g_pAIInvestLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (linkId)
      g_pAIInvestLinks->Remove(linkId);

   if (to)
      g_pAIInvestLinks->Add(GetID(), to);
}

///////////////////////////////////////////////////////////////////////////////
