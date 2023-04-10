/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ai.cpp,v 1.117 2000/03/21 12:24:11 bfarquha Exp $
//
//
// AI Flow Notes
// -------------
// The following callbacks are "unordered" and should be assumed to come
// at any time
//
//   IAIComponent:: OnGameEvent()
//
// These callbacks are always received in the following order. Some may
// not be dispatched if the AI is asleep or efficient, or if there is
// nothing to notify about. Note suggestion functions are called only
// if signalled, and signals to not carry accross frames.
//
//   IAIComponent:: OnSimStart()
//   - - - - - - - - - - - - - - - - - - - - - - - -
//   IAIComponent:: OnModeChange() (to/from efficiencies)
//   IAIComponent:: OnUpdateProps() (from)
//   IAIComponent:: OnBeginFrame()
//   IAIComponent:: OnAlertness()
//   IAIComponent:: OnFrustration()
//   IAIAction::    Update()
//   IAIComponent:: OnActionProgress()
//   IAIComponent:: OnModeChange() (to dead)
//   IAIAbility::   UpdateGoal()
//   IAIComponent:: OnGoalProgress()
//   IAIAbility::   SuggestMode()
//   IAIComponent:: OnModeChange() (based on ability desires)
//   IAIAbility::   SuggestGoal()
//   IAIComponent:: OnGoalChange()
//   IAIAbility::   SuggestAction()
//   IAIComponent:: OnActionChange()
//   IAIAction::    Enact()
//   IAIComponent:: OnEndFrame()
//   IAIComponent:: OnUpdateProps() (to)
//   - - - - - - - - - - - - - - - - - - - - - - - -
//   IAIComponent:: OnSimEnd()
//

// #define PROFILE_ON 1

#include <lg.h>
#include <str.h>
#include <appagg.h>

#include <dmgbase.h>
#include <link.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <playrobj.h>
#include <relation.h>
#include <playtest.h>   // for hello_debugger
#include <objedit.h>    // for ObjWarnName

#include <ai.h>
#include <aialert.h>
#include <aiaware.h>
#include <aiapi.h>
#include <aiapiabl.h>
#include <aiapiact.h>
#include <aiapimov.h>
#include <aiapinet.h>
#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiapibhv.h>
#include <aibasact.h>
#include <aidebug.h>
#include <aidist.h>
#include <aifreeze.h>
#include <aigoal.h>
#include <aiman.h>
#include <aiprcore.h>
#include <aipthloc.h>
#include <aisched.h>
#include <aisignal.h>
#include <aisndtyp.h>
#include <aitagtyp.h>
#include <aiteams.h>

// for medium transition:
#include <scrptapi.h>
#include <mtscrpt.h>
#include <wrfunc.h>

// for Frustration path:
#include <aiapipth.h>

#include <aiokbits.h>

#include <culpable.h>

// @HACK: This is for the camera hack for efficiency
#include <camera.h>

// @HACK: this is included for the ObjRef hack in UpdateObjState, remove when that hack is gone
#include <linkman.h>
#include <rendprop.h>

// @TODO:  Bodisafa 12/8/1999
// TEMP CODE
#include <crwpnlst.h>

// for rendering tests
#include <rendobj.h>
#include <sndframe.h>

// restructure modules so include this header after e3
#include <aicombat.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Debugging globals
//

ObjID g_AIBreakID        = OBJ_NULL;

#define AIBreakIfSet() if (GetID() != g_AIBreakID) ; else { hello_debugger(); g_AIBreakID = OBJ_NULL; }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAI
//

cAI::cAI()
 : m_SimStartTimer(0, 2000)
{
   m_pSenses = NULL;
   m_pMoveEnactor = NULL;
   m_pSoundEnactor = NULL;
   m_pPathfinder = NULL;
   m_pCurAbilityInfo = NULL;
   m_pCurGoal = NULL;
   m_CurActionLastResult = kAIR_Undefined;
   m_CurGoalLastResult = kAIR_Undefined;
   memset(&m_state, 0, sizeof(m_state));
   ((sAIState*)(void *)&m_state)->nCurrentMedium = -1;
   m_state.SetMode(kAIM_Normal);
   m_pBehaviorSet = NULL;

   m_NullActionTimer.Set(kAIT_2Hz);

   m_flags = 0;

#ifndef SHIP
   m_pDebugMoveSuggestions = 0;
   m_pDebugMoveGoal = 0;
#endif
}

///////////////////////////////////////

cAI::~cAI()
{
   int i;

   SafeRelease(m_pCurGoal)
   for (i = 0; i < m_CurActions.Size(); i++)
      m_CurActions[i]->Release();
   m_CurActions.SetSize(0);

   for (i = 0; i < m_Abilities.Size(); i++)
      SafeRelease(m_Abilities[i].pGoal);

   SafeRelease(m_pBehaviorSet);
// @TBD (toml 04-09-98):    SafeRelease(m_pCurAction);
// @TBD (toml 04-09-98): iterate and release goal pointers

   for (i = 0; i < m_OtherComponents.Size(); i++)
      SafeRelease(m_OtherComponents[i]);

   SafeRelease(m_pPathfinder);
   SafeRelease(m_pSoundEnactor);
   SafeRelease(m_pMoveEnactor);
   SafeRelease(m_pSenses);

   for (i = 0; i < m_Abilities.Size(); i++)
      SafeRelease(m_Abilities[i].pAbility);

   for (i = 0; i < m_MoveRegulators.Size(); i++)
      SafeRelease(m_MoveRegulators[i]);

}

///////////////////////////////////////

STDMETHODIMP cAI::QueryInterface(REFIID id, void ** ppI)
{
   if (IsEqualGUID(id, IID_IAI))
   {
      *ppI = (IAI *)this;
      return S_OK;
   }
   return cCTUnaggregated<IInternalAI, &IID_IInternalAI, kCTU_Default>::QueryInterface(id, ppI);
}

///////////////////////////////////////
//
// Get another AI
//

STDMETHODIMP_(IInternalAI *) cAI::AccessOtherAI(ObjID objIdOther)
{
   return g_pAIManager->AccessAI(objIdOther);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::AccessAIs(IInternalAI *** ppAIs, int * pNum)
{
   *pNum = g_pAIManager->AccessAIs().Size();
   *ppAIs = (IInternalAI **)g_pAIManager->AccessAIs().AsPointer();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::Kill()
{
   LinkDestroyMany(GetID(), LINKOBJ_WILDCARD, g_pAIAttackLinks->GetID());
   SetModeAndNotify(kAIM_Dead);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::Resurrect()
{
   SetModeAndNotify(kAIM_Normal);
}

STDMETHODIMP_(BOOL) cAI::IsDying(void)
{
   return (m_pCurGoal && (m_pCurGoal->GetType() == kAIGT_Die));
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::IsDead()
{
   return m_state.IsDead();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::Freeze(tSimTime duration)
{
   AISetFrozen(GetID(), duration);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::Unfreeze(void)
{
   AIClearFrozen(GetID());
}

///////////////////////////////////////

void cAI::SetModeAndNotify(eAIMode mode)
{
   if (m_state.GetMode() != mode)
   {
      eAIMode previous = m_state.GetMode();
      m_state.SetMode(mode);
      g_pAIModeProperty->Set(GetID(), m_state.GetMode());
      NotifyModeChange(previous, mode);
      //mprintf("%d Change mode %s -->%s!\n", GetID(), AIGetModeName(previous), AIGetModeName(mode));
   }
}

///////////////////////////////////////

void cAI::ConnectComponent(IAIComponent * pComponent)
{
   int i = m_NotifyInfo.Grow();

   m_NotifyInfo[i].pComponent     = pComponent;
   m_NotifyInfo[i].pNotifications = 0;

   pComponent->ConnectComponent(this, &(m_NotifyInfo[i].pNotifications));
}

///////////////////////////////////////
//
// Plug the AI together
//

STDMETHODIMP cAI::Init(ObjID id, IAIBehaviorSet * pBehaviorSet)
{
   m_state.SetID(id);

   m_pBehaviorSet = pBehaviorSet;
   m_pBehaviorSet->AddRef();

   //
   // Gather and connect components
   //

   cAIComponentPtrs components;
   int i;

   m_pBehaviorSet->CreateComponents(&components);

   // Get any interior components (loop continues until components array stops growing)
   for (i = 0; i < components.Size(); i++)
      components[i]->GetSubComponents(&components);


   //
   for (i = 0; i < components.Size(); i++)
   {
      // This is a bit brute force, but is really convenient...
      AutoConIPtr(AISenses, components[i]);       // Note: AutoConIPtr is an auto-QI self releasing pointer
      AutoConIPtr(AIMoveRegulator, components[i]);
      AutoConIPtr(AIAbility, components[i]);
      AutoConIPtr(AIMoveEnactor, components[i]);
      AutoConIPtr(AISoundEnactor, components[i]);
      AutoConIPtr(AIComponent, components[i]);
      AutoConIPtr(AIPathfinder, components[i]);

      AssertMsg(!!pAIComponent, "Non-component found in component set!");

      // As components come out of CreateComponents(), we need not add ref here (toml 05-18-98)

      // Plug it in
      ConnectComponent(pAIComponent);

      // Custom connections
      if (!!pAISenses)
      {
         AssertMsg(!m_pSenses, "Only one sense component be specified per AI");
         m_pSenses = pAISenses;
      }
      else if (!!pAIMoveRegulator)
      {
         m_MoveRegulators.Append(pAIMoveRegulator);
      }
      else if (!!pAIAbility)
      {
         int iNew = m_Abilities.Grow();

         m_Abilities[iNew].pAbility                = pAIAbility;
         m_Abilities[iNew].pSignals                = 0;
         m_Abilities[iNew].pGoal                   = NULL;
         m_Abilities[iNew].modeSuggestion.mode     = kAIM_Normal; // all abilities start by default suggesting normal mode
         m_Abilities[iNew].modeSuggestion.priority = kAIP_VeryLow;
         pAIAbility->ConnectAbility(&(m_Abilities[iNew].pSignals));
      }
      else if (!!pAIMoveEnactor)
      {
         AssertMsg(!m_pMoveEnactor, "Only one move enactor may be specified per AI");
         m_pMoveEnactor = pAIMoveEnactor;
      }
      else if (!!pAISoundEnactor)
      {
         AssertMsg(!m_pSoundEnactor, "Only one sound enactor may be specified per AI");
         m_pSoundEnactor = pAISoundEnactor;
      }
      else if (!!pAIPathfinder)
      {
         AssertMsg(!m_pPathfinder, "Only one pathfinder may be specified per AI");
         m_pPathfinder = pAIPathfinder;
      }
      else
      {
         m_OtherComponents.Append(pAIComponent);
      }

   }

   //
   // Start up components
   //

   if (m_pSenses)
      m_pSenses->Init();

   for (i = 0; i < m_MoveRegulators.Size(); i++)
      m_MoveRegulators[i]->Init();

   for (i = 0; i < m_Abilities.Size(); i++)
      m_Abilities[i].pAbility->Init();

   if (m_pMoveEnactor)
      m_pMoveEnactor->Init();

   if (m_pSoundEnactor)
      m_pSoundEnactor->Init();

   for (i = 0; i < m_OtherComponents.Size(); i++)
      m_OtherComponents[i]->Init();

   AssertMsg(m_Abilities.Size() <= kAIMaxAbilsPerAI, "Exceeded maximum number of per-AI abilities");

   // Add movement regulators to movement enactor
   // @TBD (toml 05-21-98): this could be redone a la the combat subabilities
   if (m_pMoveEnactor)
   {
      for (i = 0; i < m_MoveRegulators.Size(); i++)
         m_pMoveEnactor->InstallRegulator(m_MoveRegulators[i]);
   }

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAI::End()
{
   // Use the notify info table, as it has all the components in a convenient form
   for (int i = 0; i < m_NotifyInfo.Size(); i++)
   {
      m_NotifyInfo[i].pComponent->End();
   }
   return S_OK;
}

///////////////////////////////////////
//
// Access a component
//

STDMETHODIMP cAI::GetComponent(const char * pszComponent, IAIComponent ** ppComponent)
{
   // Use the notify info table, as it has all the components in a convenient form
   for (int i = 0; i < m_NotifyInfo.Size(); i++)
   {
      if (!stricmp(m_NotifyInfo[i].pComponent->GetName(), pszComponent))
      {
         *ppComponent = m_NotifyInfo[i].pComponent;
         m_NotifyInfo[i].pComponent->AddRef();
         return S_OK;
      }
   }

   *ppComponent = NULL;
   return S_FALSE;
}

///////////////////////////////////////
//
// Component iteration protocol
//

enum eAIWhichGroup
{
   kStartGroup,
   kSenseGroup,
   kMoveRegGroup,
   kAbilityGroup,
   kMoveGroup,
   kSoundGroup,
   kOtherGroup,
   kEndGroup,
};

///////////////////

struct sAICompIter
{
   eAIWhichGroup next;
   int           index;
};

///////////////////

STDMETHODIMP_(IAIComponent *) cAI::GetFirstComponent(tAIIter * pAbstractIter)
{
   sAICompIter    iter = { kStartGroup, 0 };
   tAIIter        abstractIter = (tAIIter)&iter;
   IAIComponent * pResult;

   pResult = cAI::GetNextComponent(&abstractIter);

   if (!pResult)
      return NULL;

   sAICompIter * pIter = new sAICompIter;

   *pIter = iter;
   *pAbstractIter = (tAIIter)pIter;
   return pResult;
}

///////////////////////////////////////

STDMETHODIMP_(IAIComponent *) cAI::GetNextComponent(tAIIter * pAbstractIter)
{
   sAICompIter *  pIter = (sAICompIter *)(*pAbstractIter);
   IAIComponent * pResult = NULL;

   switch (pIter->next)
   {
      case kStartGroup:
         pIter->next  = kSenseGroup;
         pIter->index = 0;
         // Fall through

      case kSenseGroup:
         if (pIter->index == 0)
         {
            pResult = m_pSenses;
            break;
         }
         // else fall through
         pIter->next  = kMoveRegGroup;
         pIter->index = 0;

      case kMoveRegGroup:
         if (pIter->index < m_MoveRegulators.Size())
         {
            pResult = m_MoveRegulators[pIter->index];
            break;
         }
         // else fall through
         pIter->next  = kAbilityGroup;
         pIter->index = 0;

      case kAbilityGroup:
         if (pIter->index < m_Abilities.Size())
         {
            pResult = m_Abilities[pIter->index].pAbility;
            break;
         }
         // else fall through
         pIter->next  = kMoveGroup;
         pIter->index = 0;

      case kMoveGroup:
         if (pIter->index == 0)
         {
            pResult = m_pMoveEnactor;
            break;
         }
         // else fall through
         pIter->next  = kSoundGroup;
         pIter->index = 0;
         break;

      case kSoundGroup:
         if (pIter->index == 0)
         {
            pResult = m_pSoundEnactor;
            break;
         }
         // else fall through
         pIter->next  = kOtherGroup;
         pIter->index = 0;
         break;

      case kOtherGroup:
         if (pIter->index < m_OtherComponents.Size())
         {
            pResult = m_OtherComponents[pIter->index];
            break;
         }
         // else fall through
   }

   if (pResult)
   {
      pIter->index++;
      pResult->AddRef();
   }
   else
      pIter->next = kEndGroup;

   return pResult;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::GetComponentDone(tAIIter * pAbstractIter)
{
   if (pAbstractIter)
      delete (sAICompIter *)(*pAbstractIter);
}

///////////////////////////////////////

STDMETHODIMP_(IAIMoveEnactor *) cAI::AccessMoveEnactor()
{
   return m_pMoveEnactor;
}

///////////////////////////////////////

STDMETHODIMP_(IAISoundEnactor *) cAI::AccessSoundEnactor()
{
   return m_pSoundEnactor;
}

///////////////////////////////////////

STDMETHODIMP_(IAISenses *) cAI::AccessSenses()
{
   return m_pSenses;
}

///////////////////////////////////////

STDMETHODIMP_(IAIPathfinder *) cAI::AccessPathfinder()
{
   return m_pPathfinder;
}

///////////////////////////////////////

STDMETHODIMP_(IAIBehaviorSet *) cAI::AccessBehaviorSet()
{
   return m_pBehaviorSet;
}

///////////////////////////////////////
//
// Notification of entering/exiting gamemode
//

// @NOTE: This function and OnSimEnd appear to be unused. The AI manager should probably
// call these instead of NotifySim... but they aren't in the internal API and I don't feel
// like changing that right now. (Jon 5/14/99)

void cAI::OnSimStart()
{
   NotifySimStart();
}

///////////////////////////////////////

void cAI::OnSimEnd()
{
   NotifySimEnd();
}

///////////////////////////////////////
//
// Get the AIs ObjID
//

STDMETHODIMP_(ObjID) cAI::GetObjID()
{
   return GetID();
}

///////////////////////////////////////
//
// Access the state of the AI
//

STDMETHODIMP_(const cAIState *) cAI::GetState()
{
   return &m_state;
}

///////////////////////////////////////
//
// Freshen the AI path cell
//

STDMETHODIMP_(tAIPathCellID) cAI::UpdatePathCell()
{
   return m_state.SetPathCell(AIFindClosestCell(*m_state.GetLocation(), m_state.GetPathCell(), GetID()));
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::GetRecentCells(cAICellArray * pResult)
{
   unsigned count = m_CellStack.GetCount();
   if (count)
   {
      pResult->SetSize(count);
      m_CellStack.PeekAll(pResult->AsPointer());
   }
   return;
}

///////////////////////////////////////

STDMETHODIMP_(float) cAI::GetGroundOffset()
{
   float base = AIGetGroundOffset(GetID());
   if (m_state.GetMode() == kAIM_Efficient && base == 0.0)
      return 4.0;

   return base;
}

///////////////////////////////////////

#define kDeltaAllowed 1.9

float cAI::AdjustMvrGroundHeight(const Location * pLoc, float height, ObjID htObj)
{
   mxs_vector    loc;
   tAIPathCellID cell;
   sAIState * pState = (sAIState *)((void *)&m_state); // cheating for expedience. should clean this up after ship (toml 11-04-98)
   if (pState->cell)
   {
      if (AIFindClosestConnectedCell(pState->cell, pState->pathLoc, pLoc->vec, &cell, &loc, pState->nStressBits))
      {
         if (loc.z > height && loc.z - height > kDeltaAllowed)
         {
            AIWatch2(RecoverPath, AIDebugGet(), "AIAdjustMvrGroundHeight() Suggesting ground of %g instead of %g", loc.z, height);
            height = loc.z;
         }
      }
      else if (m_ResetTimer.Expired())
      {
         ResetGoals();
         m_ResetTimer.Set(AICustomTime(30000));
      }
   }
   return height;
}

///////////////////////////////////////
//
// Sense accessor
//
// Never returns NULL. Returned structure should never be considered
// valid across calls to this function or across frames.
//

STDMETHODIMP_(const sAIAwareness *) cAI::GetAwareness(ObjID target)
{
   if (m_pSenses)
      return m_pSenses->GetAwareness(target);

   return AINullAware(target);
}

///////////////////////////////////////
//
// Utility to get the closest player.  Most useful in networked games.
//

STDMETHODIMP_(ObjID) cAI::GetClosestPlayer()
{
   AutoAppIPtr(AINetServices);

   tAIPlayerList players;
   ObjID         closestPlayer;

   if (pAINetServices->GetPlayers(&players) > 1)
   {
      float      distSq;
      float      closestDistSq = m_state.DistSq(*GetObjLocation(players[0]));
      ObjID *    pPlayerID     = &players[1];

      closestPlayer = players[0];

      while (*pPlayerID)
      {
         distSq = m_state.DistSq(*GetObjLocation(*pPlayerID));

         if (distSq < closestDistSq)
         {
            closestPlayer = *pPlayerID;
            closestDistSq = distSq;
         }
         pPlayerID++;
      }
   }
   else
      closestPlayer = players[0];

   return closestPlayer;
}

///////////////////////////////////////
//
// Get the efficiency. Note, the pointer returned is a temporary
//

const sAIEfficiency * cAI::GetEfficiency()
{
   return AIGetEfficiency(GetID());
}

///////////////////////////////////////

void cAI::ResetGoals()
{
   m_flags |= kResetGoals;
}

///////////////////////////////////////

void cAI::CheckGoalsReset()
{
   if (m_flags & kResetGoals)
   {
#if 0
      if (m_pCurGoal)
      {
         AIWatch(Flow, GetID(), "Resetting goals");
         AIWatch(RecoverPath, GetID(), "Resetting goals");

         NotifyGoalsReset(m_pCurGoal);

         m_pCurGoal->pOwner->SetControl(FALSE);
         m_pCurAbilityInfo = NULL;

         for (int i = 0; i < m_CurActions.Size(); i++)
            m_CurActions[i]->Release();
         m_CurActions.SetSize(0);

         cAIGoal * pPrevGoal = m_pCurGoal;
         m_pCurGoal = NULL;
         m_CurGoalLastResult = kAIR_Undefined;

         NotifyGoalChange(pPrevGoal, NULL);
      }

      for (int i = 0; i < m_Abilities.Size(); i++)
      {
         *(m_Abilities[i].pSignals) |= kAI_SigGoal;
      }
#endif
      m_flags &= ~kResetGoals;
   }
}

///////////////////////////////////////
//
// Run one frame
//

DECLARE_TIMER(AI_AI_ONF, Average);

HRESULT cAI::OnNormalFrame()
{
   AUTO_TIMER(AI_AI_ONF);

   AIDebugSet(GetID());
   AIBreakIfSet();

   if (!(m_flags & kStarted) && !m_SimStartTimer.Expired()) // First time through UpdateState() will set started flag
      return S_OK;

   // Synchronize state structure with world database
   UpdateState();

   // Check if the AI should cycle...
   if (m_state.GetFlags() & (kAI_IsRemote | kAI_OutOfWorld))
      return S_FALSE;

   if (m_state.GetMode() == kAIM_Asleep || m_state.GetMode() == kAIM_Dead)
      return S_OK;

   //
   // Update efficiency status
   //
   UpdateEfficiency();

   // If the AI is super efficient, we're done
   if (m_state.GetMode() < kAIM_Efficient)
      return S_OK;

   //
   // Properties may be cached locally -- update data from properties
   //
   UpdateProperties(kAI_FromProps);

   //
   // Dispatch frame start to components
   //
   NotifyBeginFrame();

   //
   // Update AI senses and status, generating relevant events, setting appropriate signals
   //
   if (UpdateSenses() == S_OK)
   {
      //
      // Based on sense/awareness, determine the current alertness
      //
      UpdateAlertness();
   }

   // See if we're frustrated:
   UpdateFrustration();

   //
   // Check for goal reset
   //
   CheckGoalsReset();

   //
   //
   //
   if (UpdateAction() > kAIR_NoResult)
   {
      if (m_state.GetMode() == kAIM_Dead)
         return S_OK;
      UpdateGoal();
   }

   //
   // If any signals set at this point, there are decisions to make
   //
   if (CheckSignals())
   {
      //
      // Run decision process, according to signals that indicate abilities
      // have something useful to say, then enact the action(s).
      //
      DecideMode();
      DecideGoal();
      if (!(m_state.GetFlags() & kAI_IsBrainsOnly))
         DecideAction();

      ClearSignals();
   }

   Enact();

   //
   // Dispatch frame end to components
   //
   NotifyEndFrame();

   //
   // Refresh the property system
   //
   UpdateProperties(kAI_ToProps);

   return S_OK;
}

///////////////////////////////////////
//
// Determine if the AI should enter or exit efficiency modes
//

DECLARE_TIMER(AI_AIF_UE, Average);

#define kMinTimeNormalAfterRender 10000

#ifndef SHIP
eAIMode g_ForceAIMode = kAIM_Num;
#endif

eAIMode cAI::UpdateEfficiency()
{
   AUTO_TIMER(AI_AIF_UE);
   const cMxsVector *    pLoc;
   cMxsVector            playerLoc;
   float                 playerDistSq;
   float                 zDiff;
   eAIMode               mode;
   const sAIEfficiency * pEfficiency;

#ifndef SHIP
   if (g_ForceAIMode < kAIM_Num)
   {
      SetModeAndNotify(g_ForceAIMode);
      return g_ForceAIMode;
   }
#endif

   mode = m_state.GetMode();

   if (!m_UpdateSchedule.efficiency.Expired())
      return mode;

   m_UpdateSchedule.efficiency.Reset();

   pEfficiency = GetEfficiency();

   // If we don't support efficiency modes, make sure we're not in one, else
   // compute the distance to the closest player
   if (WasRendered(kMinTimeNormalAfterRender) || !pEfficiency->fEnabled || (m_pCurGoal && m_pCurGoal->GetType() == kAIGT_Die))
   {
      if (mode > kAIM_Asleep && mode < kAIM_Normal)
      {
         SetModeAndNotify(kAIM_Normal);
         return kAIM_Normal;
      }
      else
         return mode;
   }

   ObjID player = GetClosestPlayer();
   GetObjLocation(player, &playerLoc);
   Camera* playcam = PlayerCamera();
   //@HACK: if not in FIRST_PERSON mode, use the camera location for
   //efficency mode.  Of course, this leads to questions like:
   //Why don't we always use the camera?  What about multiplayer?
   //But this change should not break Multiplayer that isn't using
   //remote cameras, and should fix single player that is.
   if (CameraGetMode(playcam) != FIRST_PERSON)
     {
       mxs_angvec throwang;
       CameraGetLocation(playcam,&playerLoc,&throwang);
     }
   pLoc = m_state.GetLocation();

   playerDistSq = AIDistanceSq(*pLoc, playerLoc);
   zDiff        = ffabsf(pLoc->z - playerLoc.z);

   // If we're not efficient, should we be?
   if (mode > kAIM_SuperEfficient && /*!g_AIScheduleSettings.fActive &&*/
       (playerDistSq > pEfficiency->distEnterSuperEffSq ||
        zDiff > pEfficiency->zEnterSuperEff))
   {
      mode = kAIM_SuperEfficient;
   }
   else if (mode > kAIM_Efficient &&
            (playerDistSq > pEfficiency->distEnterEffSq ||
             zDiff > pEfficiency->zEnterEff))
   {
      mode = kAIM_Efficient;
   }

   // Otherwise, should we exit an efficieny mode?
   else if (mode < kAIM_Normal)
   {
      if (/*g_AIScheduleSettings.fActive ||*/
          (playerDistSq < pEfficiency->distExitSuperEffSq &&
           zDiff < pEfficiency->zExitSuperEff))
      {
         mode = kAIM_Efficient;
      }

      if (playerDistSq < pEfficiency->distExitEffSq &&
          zDiff < pEfficiency->zExitEff)
      {
         mode = kAIM_Normal;
      }
   }

#ifndef SHIP
   // Debug mode randomizer
   if (g_AIEfficiencyDebug && mode != kAIM_Combat)
   {
      mode = (eAIMode)AIRandom(kAIM_SuperEfficient, kAIM_Normal);
      m_UpdateSchedule.efficiency.Set(AICustomTime(4000));
   }
#endif

   SetModeAndNotify(mode);

   return mode;
}

///////////////////////////////////////
//
// Update cached, generally read only, state data
//

DECLARE_TIMER(AI_AIF_US, Average);

void cAI::UpdateState()
{
   AUTO_TIMER(AI_AIF_US);

   sAIState * pState = (sAIState *)((void *)&m_state);     // Naughty, but handy (toml 04-07-98)
   mxs_vector prevLoc = pState->loc;

   if (!GetObjPosition(pState->id, &pState->position))
   {
      pState->flags |= kAI_OutOfWorld;
      return;
   }

   pState->flags &= ~kAI_OutOfWorld;

   if (!(m_flags & kStarted))
   {
      BOOL bVal;

      pState->pathLoc = pState->loc;
      if ((pState->cell = AIFindClosestCell(pState->loc, 0, pState->id)) != 0)
         m_CellStack.Push(&pState->cell);
      m_flags |= kStarted;

      // Just do this when we start. If we really want to update this as we go, we can do it every frame, or prop listener.
      pState->bNeedsLargeDoors = AINeedsLargeDoors(GetID(), bVal);
   }
   else
   {
      #define kTeleportResetDistSq (sq(6.0))
      tAIPathCellID prevCell = pState->cell;
      if (memcmp(&pState->loc, &prevLoc, sizeof(mxs_vector)) != 0 || !pState->cell)
      {
         mxs_vector delta;
         float      distSq;
         mx_sub_vec(&delta, &pState->loc, &prevLoc);
         distSq = mx_mag2_vec(&delta);

         if (distSq > kTeleportResetDistSq)
         {
            pState->pathLoc = pState->loc;
            m_CellStack.Flush();
            prevCell = 0;
            pState->cell = AIFindClosestCell(pState->loc, 0, pState->id);
            ResetGoals();
         }
         else if (!pState->cell)
         {
            pState->pathLoc = pState->loc;
            pState->cell = AIFindClosestCell(pState->loc, 0, pState->id);
         }
         else
         {
            mxs_vector    newLoc;
            tAIPathCellID newCell;

            if (AIFindClosestConnectedCell(pState->cell, pState->pathLoc, pState->loc, &newCell, &newLoc, pState->nStressBits))
            {
               pState->pathLoc = newLoc;
               pState->cell = newCell;
            }
            else
            {
               AIWatch(RecoverPath, AIDebugGet(), "Failed to determine new cell. Resync!");
               pState->pathLoc = pState->loc;
               pState->cell = AIFindClosestCell(pState->loc, pState->cell, pState->id);
            }
         }
      }
      if ((!prevCell || (prevCell != pState->cell)) && pState->cell)
      {
         m_CellStack.Push(&pState->cell);
      }
   }

   GetObjFacing(pState->id, &pState->facingAng);

   eAIMode prevMode = pState->mode;
   pState->mode = AIGetMode(GetID());

   // This doesn't really need to be done every frame but it is fairly cheap
   if ((m_pMoveEnactor) && m_pMoveEnactor->SupportsMotionActions())
      m_state.SetFlags(kAI_IsCreature);

   if (prevMode != pState->mode)
   {
      //mprintf("%d Change mode (inconsistent) %s -->%s!\n", GetID(), AIGetModeName(prevMode), AIGetModeName(pState->mode));
      NotifyModeChange(prevMode, pState->mode);
   }

   // Lastly, check our media. If it has changed since last frame, and this ai wants to track
   // media changes, then send a script message.
   BOOL bVal = FALSE;
   if (g_pAITrackMediumProperty && g_pAITrackMediumProperty->Get(pState->id, &bVal) && bVal)
   {
      int nCurrentMedium;
      Location loc;
      loc.hint = CELL_INVALID;
      loc.vec = pState->loc;
      int cell = ComputeCellForLocation(&loc);
      if (cell >=0)
         nCurrentMedium = WR_CELL(cell)->medium;
      else
         nCurrentMedium = -1;

      if (pState->nCurrentMedium != nCurrentMedium)
      {
         AutoAppIPtr(ScriptMan);

         sMediumTransMsg MTMessage(pState->id, pState->nCurrentMedium, nCurrentMedium);
         pScriptMan->SendMessage(&MTMessage);
      }
      pState->nCurrentMedium = nCurrentMedium;
   }
}

///////////////////////////////////////
//
// Update local cached property data
//

DECLARE_TIMER(AI_AIF_UP, Average);

void cAI::UpdateProperties(eAIPropUpdateKind kind)
{
   AUTO_TIMER(AI_AIF_UP);

   // AI Mode is necessarily grabbed in UpdateState() and written back if it ever changes(toml 05-21-98)
   sAIState * pState = (sAIState *)((void *)&m_state);     // Naughty, but handy (toml 04-07-98)
   if (kind == kAI_FromProps)
   {
      // For write-back, alertness is kept consistent by UpdateAlertness() (toml 05-21-98)
      Assert_(AIGetAlertness(GetID()));
      eAIAwareLevel alertness = AIGetAlertness(GetID())->level;

      pState->nStressBits = 0;

      // Use stress bits to store whether or not this ai can swim - if he has the property,
      // or for some reason finds that he's already in water, allow him to path through it:
      BOOL bCanUseWater = FALSE;
      if ((g_pAICanPathWaterProperty->Get(GetID(), &bCanUseWater) && bCanUseWater) ||
          (pState->nCurrentMedium == MEDIA_WATER))
         pState->nStressBits |= kAIOK_Swim;

//
// wsf: we're not doing alertness-based stress for now.
// Instead, every pathfind will try non-stressed first, and if it fails, then
// will try stressed. We're thinking this should not be much of a speed hit, since
// most pathfinds succeed. Also, there may be plans to make the pathfind asynchronous, which would
// prevent any big pauses.
//
#if 0
      tSimTime nCurrentTime = GetSimTime();
      if (alertness != pState->alertness)
         {
         pState->last_alertness = pState->alertness;
         pState->nLastAlertChangeTime = nCurrentTime;
         }

      if (pState->alertness >= kAIAL_Moderate)
         pState->nStressBits = kAIOKCOND_Stressed | kAIOKCOND_WasStressed;
      // If it's been more than 'n' seconds since our last mode change, and we were "stressed".
      else if ((pState->last_alertness >= kAIAL_Moderate) && ((nCurrentTime-pState->nLastAlertChangeTime) < SIM_TIME_SECOND*20))
         pState->nStressBits = kAIOKCOND_WasStressed;
#endif

// Use "stress bits" more like just "additional bits", so now it's a bit of a misnomer. We'll do this
// because we want creatures that are considered "small" to try and path using cells linked via "SmallCreature"
// links. SmallCreatures don't try to path via "stressed" links.

      // wsf: ok, maybe it's a bit wasteful to do this every frame:
      if (g_pAIIsSmallCreatureProp->IsRelevant(GetID()))
      {
         BOOL bIsSmallCreature;
         g_pAIIsSmallCreatureProp->Get(GetID(), &bIsSmallCreature);
         if (bIsSmallCreature)
            pState->nStressBits |= kAIOK_SmallCreature;
      }

      pState->alertness = alertness;
   }

   // Try components...
   NotifyUpdateProperties(kind);
}

///////////////////////////////////////
//
// Update sensory data
//

DECLARE_TIMER(AI_AIF_USNS, Average);

HRESULT cAI::UpdateSenses()
{
   AUTO_TIMER(AI_AIF_USNS);

   if (m_pSenses)
      return m_pSenses->Update();
   return S_FALSE;
}


///////////////////////////////////////
//
//
//


void cAI::UpdateFrustration()
{
   sAIFrustrated *temp;
   // If we have a "frustration" property, then we are frustrated.
   if (g_pAIFrustratedProperty->Get(GetID(), &temp))
      {
      NotifyFrustration(temp);
      g_pAIFrustratedProperty->Delete(GetID()); // remove frustration notification.
      }
}



static LinkID GetAwarenessLink(ObjID aiid, ObjID objId, sAIAwareness ** ppAwareness)
{
   //
   // Grab any existing sense data, or use temporary
   //
   static sAIAwareness awarenessBuf;
   LinkID              linkID       = (objId) ? g_pAIAwarenessLinks->GetSingleLink(aiid, objId) : NULL;

   if (linkID)
   {
      *ppAwareness = (sAIAwareness *)g_pAIAwarenessLinks->GetData(linkID);
      (*ppAwareness)->object = objId;
      (*ppAwareness)->linkID = linkID;
   }
   else
      *ppAwareness = 0;

   return linkID;
}



///////////////////////////////////////
//
//
//

// @TBD (toml 05-23-98): propertize this constant
int g_AITimeMaxAlert = 30000;

DECLARE_TIMER(AI_AIF_UALT, Average);

void cAI::UpdateAlertness()
{
   AUTO_TIMER(AI_AIF_UALT);

   //
   // Update the alertness
   //
   sAIAlertness *       pAlertness = AIGetAlertness(GetID());
   const sAIAwareness * pHighestAwareness = (m_pSenses) ? m_pSenses->GetHighAwareness(kAISGH_Alerting) : NULL;
   ObjID                source;

   AssertMsg(pAlertness, "Expected alertness property");

   eAIAlertLevel previous = pAlertness->level;
   eAIAlertLevel targetLevel;

   sAIState * pState = (sAIState *)((void *)&m_state);     // Naughty, but handy (toml 04-07-98)

   // If we're in maximum alert, we stay there for a period of time
   if (pHighestAwareness &&
       pAlertness->level == kAIAL_High &&
       pHighestAwareness->TimeSinceContact() < g_AITimeMaxAlert)
   {
      targetLevel = kAIAL_High;
      source      = pHighestAwareness->object;
   }
   // Otherwise we inherit the highest awareness
   else if (pHighestAwareness)
   {
      targetLevel = pHighestAwareness->level;
      source      = pHighestAwareness->object;
   }
   else
   {
      targetLevel = kAIAL_Lowest;
      source      = OBJ_NULL;
   }

   // Enforce min/max restictions...

   const sAIAlertCap * pAlertCap = AIGetAlertCap(GetID());

   if (targetLevel < pAlertCap->minLevel)
      targetLevel = pAlertCap->minLevel;
   else if (targetLevel > pAlertCap->maxLevel)
      targetLevel = pAlertCap->maxLevel;

   // If the AI has become very excited, it may never return to entirely calm...
   if (pAlertness->peak == kAIAL_High &&
       targetLevel < pAlertCap->minRelax)
   {
      targetLevel = pAlertCap->minRelax;
   }

#if 0
   //
   // Handle case of surprise
   //
   if (IsAPlayer(source) && target == kAIAL_High && pAlertness->level == kAIAL_Lowest)
   {
      AIWatch(Alert, GetID(), "Surprised");
      ; //NotifySuprise(pSourceAwareness);
   }
#endif

   pAlertness->level = targetLevel;

   if (pAlertness->level > pAlertness->peak)
      pAlertness->peak = pAlertness->level;


   // See if this is first time AI is highly alerted to player
   sAIAwareness * pPlayerAwareness = 0;
   LinkID         playerLinkID;
   playerLinkID = GetAwarenessLink(GetID(), PlayerObject(), &pPlayerAwareness);
   if (pPlayerAwareness && pPlayerAwareness->bDidHighAlert)
   {
      if (!pState->bDidHighAlert)
      {
         pState->bDidHighAlert = TRUE;
         NotifyHighAlert(source, previous, pAlertness->level, pAlertness);
      }
   }

#ifndef SHIP
   if (AIIsWatched(Alert, GetID()) && g_fAIAwareOfPlayer)
   {
      static cAITimer timer(kAIT_1Sec);
      if (timer.Expired())
      {
         AIWatch3(Alert,
                  GetID(),
                  "Lev \"%s\", Src %d, Awr \"%s\"",
                  AIGetAlertLevelName(pAlertness->level),
                  source,
                  AIGetAwareLevelName((pHighestAwareness) ? pHighestAwareness->level : kAIAL_Lowest));

         timer.Reset();
      }
   }
#endif

   //
   // Notify clients and update the alertness property
   //
   if (previous != pAlertness->level)
   {
      pState->alertness = pAlertness->level;
      NotifyAlertness(source, previous, pAlertness->level, pAlertness);
   #ifndef SHIP
      AIWatch2(Alert, GetID(), "Alertness %d ==> %d", previous, pAlertness->level);
   #endif
   }

   // We've just become aware of something, or regained contact with it.
   if (pHighestAwareness &&
       ((pHighestAwareness->TimeSinceContact() > 0) || !(pHighestAwareness->flags & kAIAF_HaveLOS))) // we've lost it
      pState->bHighAwareLostContact = TRUE;

   // We're considered to have "regained" contact if our TimeSinceContact is 0, and we have LOS.
   if ((source != pState->LastHighAwareObject) ||
       (pState->bHighAwareLostContact &&
        pHighestAwareness &&
       (!pHighestAwareness->TimeSinceContact() && (pHighestAwareness->flags & kAIAF_HaveLOS))))
   {
      pState->LastHighAwareObject = source;
      NotifyAwareness(source, pAlertness->level, pAlertness);
   }

   if (pHighestAwareness &&
       ((pHighestAwareness->TimeSinceContact() == 0) && (pHighestAwareness->flags & kAIAF_HaveLOS))) // we've found it
      pState->bHighAwareLostContact = FALSE;

   //
   // Update the alertness property
   //
   g_pAIAlertnessProperty->Set(GetID(), pAlertness);
}

///////////////////////////////////////
//
// Update current action, if any
//

DECLARE_TIMER(AI_AIF_UA, Average);

eAIResult cAI::UpdateAction()
{
   AUTO_TIMER(AI_AIF_UA);

   int  i;
   BOOL fInProgress = FALSE;

   fInProgress = m_CurActions.Size() > 0;

   if (!fInProgress)
   {

      if (m_NullActionTimer.Expired() &&
         (!m_pMoveEnactor || m_pMoveEnactor->GetStatus() < kAIME_ActiveBusy))
      {
         m_NullActionTimer.Reset();
         cAINullAction nullAction((m_pCurGoal) ? m_pCurGoal->pOwner : NULL);
         NotifyActionProgress(&nullAction);
      }
      return kAIR_Nothing;
   }

   eAIResult result = kAIR_NoResult;
   eAIResult currentResult;

   for (i = 0; i < m_CurActions.Size(); i++)
   {
      currentResult = m_CurActions[i]->Update();

      if (currentResult != kAIR_NoResult)
      {
         BOOL fNotify = (currentResult == kAIR_NoResultSwitch || currentResult != m_CurActionLastResult);
         m_CurActionLastResult = currentResult;

         if (fNotify)
            NotifyActionProgress(m_CurActions[i]);
      }

      // @TBD (toml 05-23-98): multiple actions should be rethought?

      if (currentResult > result)
         result = currentResult;
   }

   return result;
}

///////////////////////////////////////
//
// Update current goal, if any
//

DECLARE_TIMER(AI_AIF_UG, Average);

eAIResult cAI::UpdateGoal()
{
   AUTO_TIMER(AI_AIF_UG);
   eAIResult currentResult;

   if (!m_pCurGoal)
      currentResult = kAIR_Nothing;

   else if (!m_pCurGoal->InProgress() || !m_UpdateSchedule.goal.Expired())
      currentResult = m_pCurGoal->result;

   else
   {
      m_UpdateSchedule.goal.Reset();

      currentResult = m_pCurGoal->pOwner->UpdateGoal(m_pCurGoal);
      if (m_CurGoalLastResult != currentResult)
      {
         NotifyGoalProgress(m_pCurGoal);
         m_CurGoalLastResult = currentResult;
      }
   }

   return currentResult;
}

////////////////////////////////////
//
// Check if any abilities have signaleld
//

DECLARE_TIMER(AI_AIF_CS, Average);

BOOL cAI::CheckSignals(tAIAbilitySignals signals)
{
   AUTO_TIMER(AI_AIF_CS);
   for (int i = 0; i < m_Abilities.Size(); i++)
   {
      if (m_Abilities[i].CheckSignals(signals))
         return TRUE;
   }
   return FALSE;
}

////////////////////////////////////
//
// Clear out ability signals
//

DECLARE_TIMER(AI_AIF_CLRS, Average);

void cAI::ClearSignals()
{
   AUTO_TIMER(AI_AIF_CLRS);
   for (int i = 0; i < m_Abilities.Size(); i++)
   {
      *(m_Abilities[i].pSignals) = 0;
   }
}

///////////////////////////////////////
//
//
//

static int ModeCompare(sAIModeSuggestion ** ppSuggest1, sAIModeSuggestion ** ppSuggest2)
{
   // @TBD (toml 04-10-98): too simple -- need tiebreaks
   return (*ppSuggest1)->priority - (*ppSuggest2)->priority;
}

///////////////////

DECLARE_TIMER(AI_AIF_DM, Average);

void cAI::DecideMode()
{
   AUTO_TIMER(AI_AIF_DM);
   // Iterate over components that have signaled...
   int i;
   BOOL fNew = FALSE;

   for (i = 0; i < m_Abilities.Size(); i++)
   {
      if (m_Abilities[i].CheckSignals(kAI_SigMode))
      {
         if (m_Abilities[i].pAbility->SuggestMode(&(m_Abilities[i].modeSuggestion)) == S_OK)
            fNew = TRUE;
      }
   }

   // If any signaled and suggested a mode, decide our new mode
   if (fNew)
   {
      sAIModeSuggestion * suggestions[kAIMaxAbilsPerAI];

      for (i = 0; i < m_Abilities.Size(); i++)
         suggestions[i] = &(m_Abilities[i].modeSuggestion);

      sAIModeSuggestion * pDecision = AIDecide(suggestions,
                                               m_Abilities.Size(),
                                               ModeCompare,
                                               NULL);

      SetModeAndNotify(pDecision->mode);
   }
}

///////////////////////////////////////
//
// AI goal resolution
//

static int GoalCompare(cAIGoal ** ppGoal1, cAIGoal ** ppGoal2)
{
   // Higher priority wins
   int result = (*ppGoal1)->priority - (*ppGoal2)->priority;

   // Else higher type wins
   if (result == 0)
      result = (*ppGoal1)->GetType() - (*ppGoal2)->GetType();

   // Else player-related goal wins
   if (result == 0)
   {
      BOOL fPlayer1 = !!IsAPlayer((*ppGoal1)->object),
           fPlayer2 = !!IsAPlayer((*ppGoal1)->object);
      result = fPlayer1 - fPlayer2;
   }

   // Otherwise, the "closest" goal wins
   if (result == 0 &&
      (*ppGoal1)->location != kInvalidLoc &&
      (*ppGoal2)->location != kInvalidLoc)
   {
      const cAIState * pAIState = (*ppGoal1)->pOwner->AccessOuterAI()->GetState();
      float            distSq1,
                       distSq2;

      distSq1 = pAIState->DistSq((*ppGoal1)->location);
      distSq2 = pAIState->DistSq((*ppGoal2)->location);

      result = pflcmp(distSq1, distSq2);
   }

   return result;
}

///////////////////

STDMETHODIMP_(void) cAI::SetGoal(cAIGoal *pDecision)
{
   // Check for change of ability
   if (!pDecision ||
       !m_pCurGoal ||
       pDecision->pOwner != m_pCurGoal->pOwner)
   {
      if (m_pCurGoal)
      {
         m_pCurGoal->pOwner->SetControl(FALSE);
         m_pCurAbilityInfo = NULL;
      }

      if (pDecision)
      {
         for (int i = 0; i < m_Abilities.Size(); i++)
         {
            if (m_Abilities[i].pAbility == pDecision->pOwner)
            {
               m_pCurAbilityInfo = &m_Abilities[i];
               break;
            }
         }

         pDecision->pOwner->SetControl(TRUE);
      }
   }

#if 0
   if (!AIInProgress(pDecision->result))
      Warning(("AI %d decided to persue a goal that has already been satisfied.\n"));
#endif

   // Check for change of goal
   if (pDecision != m_pCurGoal)
   {
      // If so, release previous goal, and any previous actions if
      // ability changed
      if (m_pCurGoal && (!pDecision ||
                         m_pCurGoal->pOwner != pDecision->pOwner))
      {
         for (int i = 0; i < m_CurActions.Size(); i++)
            m_CurActions[i]->Release();
         m_CurActions.SetSize(0);
      }

      cAIGoal * pPrevGoal = m_pCurGoal;
      m_pCurGoal = pDecision;
      if (m_pCurGoal)
      {
         m_pCurGoal->AddRef();
         if (m_pCurGoal->InProgress())
            m_CurGoalLastResult = kAIR_Undefined;
         else
            m_CurGoalLastResult = m_pCurGoal->result;
      }
      else
         m_CurGoalLastResult = kAIR_Undefined;

      NotifyGoalChange(pPrevGoal, m_pCurGoal);

      if (m_pCurGoal && m_state.GetMode() < kAIM_Normal && m_pCurGoal->GetType() == kAIGT_Die)
         SetModeAndNotify(kAIM_Normal);

      SafeRelease(pPrevGoal);
   }
}


DECLARE_TIMER(AI_AIF_DG, Average);

void cAI::DecideGoal()
{
   AUTO_TIMER(AI_AIF_DG);
   int       i;
   BOOL      fNew          = FALSE;
   BOOL      fCurChange    = FALSE;
   cAIGoal * pTieBreakGoal = NULL;

   // Iterate over components that have signaled...
   for (i = 0; i < m_Abilities.Size(); i++)
   {
      if (m_Abilities[i].CheckSignals(kAI_SigGoal))
      {
         cAIGoal * pAbilityGoal = NULL;
         cAIGoal * pAbilityPrevGoal = m_Abilities[i].pGoal;

         m_Abilities[i].pAbility->SuggestGoal(pAbilityPrevGoal, &pAbilityGoal);

         if (pAbilityPrevGoal != pAbilityGoal)
            fNew = TRUE;

         // In the event of a tie in decision, the current ability in control wins
         if (m_pCurGoal && pAbilityPrevGoal == m_pCurGoal)
         {
            pTieBreakGoal = pAbilityGoal;
         }

         SafeRelease(pAbilityPrevGoal);
         m_Abilities[i].pGoal = pAbilityGoal;
      }
   }

   // If any signaled and suggested a new goal, decide our new goal
   if (fNew)
   {
      int j;
      cAIGoal * goals[kAIMaxAbilsPerAI];

      // wsf: Here's something new. Previously, we just did this:
#if 0
      for (i = 0, j = 0; i < m_Abilities.Size(); i++)
      {
         if (m_Abilities[i].pGoal)
            goals[j++] = m_Abilities[i].pGoal;
      }

      cAIGoal * pDecision = AIDecide(goals,
                                     j,
                                     GoalCompare,
                                     pTieBreakGoal);

#else
      // wsf: Now, do an AIDecide, then call a new IAIAbility method, "FinalizeGoal". If this
      // returns FALSE, then we remove this goal from the list, and decide again. We do this
      // so that things like Fleeing, can have an efficient opportunity to see if a flee point
      // is pathable or not. If no flee point is pathable, then the Finalize will fail, and a new
      // goal will be chosen.

      BOOL bDecided = FALSE;
      cAIGoal * pDecision;
      cDynArray<int> FailedGoals; // @TBD: maybe make this a member so we don't do this all the time.

      FailedGoals.SetSize(m_Abilities.Size());
      for (i = 0; i < FailedGoals.Size(); i++)
         FailedGoals[i] = 0;

      while (!bDecided)
      {
         for (i = 0, j = 0; i < m_Abilities.Size(); i++)
         {
            if (!FailedGoals[i] && m_Abilities[i].pGoal)
               goals[j++] = m_Abilities[i].pGoal;
         }

         pDecision = AIDecide(goals,
                              j,
                              GoalCompare,
                              pTieBreakGoal);

         if (pDecision)
         {
            bDecided = pDecision->GetAbility()->FinalizeGoal(pDecision);
            // If goal decides it really doesn't want to do this, then mark goal as failed.
            if (!bDecided)
            {
               for (i = 0; i < m_Abilities.Size(); i++)
                  if (m_Abilities[i].pGoal == pDecision)
                  {
                     FailedGoals[i] = 1;
                     break;
                  }
               pDecision = 0; // wsf: this little loop is dumb.
            }
         }
         else
            bDecided = TRUE;
      }
#endif

      SetGoal(pDecision);
   }
}

///////////////////////////////////////
//
//
//

DECLARE_TIMER(AI_AIF_DA, Average);

void cAI::DecideAction()
{
   // @TBD (toml 05-23-98): once again, must emphasize how disjoint things are between cAIActions and reality
   AUTO_TIMER(AI_AIF_DA);

   if (m_pCurGoal && m_pCurAbilityInfo->CheckSignals(kAI_SigAct))
   {
      cAIActions old(m_CurActions);
      m_CurActions.SetSize(0);
      if (m_pCurGoal->pOwner->SuggestActions(m_pCurGoal, old, &m_CurActions) == S_OK)
      {
         cStr str;
         for (int i = 0; i < m_CurActions.Size(); i++)
            NotifyActionChange(old.Size() ? old[0] : NULL, m_CurActions[i]); // Not right, but servicable as long as abilities are only suggesting single actions, which is the case right now (toml 05-23-98)
      }
      m_CurActionLastResult = kAIR_Undefined;

      for (int i = 0; i < old.Size(); i++)
         old[i]->Release();
      // @TBD (toml 04-09-98): test for collisions. may need bits on action indicating enactors used
   }

   // Go through current actions and see if any want to give us a path.
   // In practice, though, there should be only action per ability.

   // Set this state info, too, while we're here:
   m_state.SetCurrentPath(0);

   for (int i = 0; (i < m_CurActions.Size()) && !m_state.GetCurrentPath(); i++)
      m_state.SetCurrentPath(m_CurActions[i]->GetPath()); // Nope, this isn't AddRef'd or Released, but should be ok, since occurs at beginning of frame.
}

///////////////////////////////////////
//
// Enact result of decision process
//
// @TBD (toml 04-04-98): much more to do here!

DECLARE_TIMER(AI_AIF_E, Average);

void cAI::Enact()
{
   AUTO_TIMER(AI_AIF_E);
   BOOL fEnacted = FALSE;

   ulong deltaTime = AIGetFrameTime();

   for (int i = 0; i < m_CurActions.Size(); i++)
   {
      if (m_CurActions[i]->InProgress())
      {
         m_CurActions[i]->Enact(deltaTime);
         fEnacted = TRUE;
      }
   }

   if (m_pMoveEnactor && !fEnacted)
   {
      m_pMoveEnactor->NoAction(deltaTime);
   }
}

///////////////////////////////////////

BOOL cAI::WasRendered(unsigned time) const
{
   return (AIGetTime() - m_TimeLastRendered <= time);
}

///////////////////////////////////////

void cAI::UpdateRenderedStatus()
{
   if (rendobj_object_visible(GetID()) || SNDFramePlayedObj(GetID()))
      m_TimeLastRendered = AIGetTime();
}

///////////////////////////////////////
//
// Lovely macros to simplify whipping up notifications
//

#define FOR_EACH_THAT_WANTS(type) \
   IAIComponent * pComponent; \
   if (g_fAIPathFindInited && m_state.GetMode() != kAIM_Asleep) \
      for (int _i = 0; _i < m_NotifyInfo.Size(); _i++) \
         if (m_NotifyInfo[_i].Wants(type) && (pComponent = m_NotifyInfo[_i].pComponent) != NULL)

#define FOR_ALL_COMPONENTS() \
   IAIComponent * pComponent; \
   for (int _i = 0; _i < m_NotifyInfo.Size(); _i++) \
      if ((pComponent = m_NotifyInfo[_i].pComponent) != NULL)


DECLARE_TIMER(AI_AI_N_SS, Average);
DECLARE_TIMER(AI_AI_N_SE, Average);
DECLARE_TIMER(AI_AI_N_D, Average);
DECLARE_TIMER(AI_AI_N_DAM, Average);
DECLARE_TIMER(AI_AI_N_W, Average);
DECLARE_TIMER(AI_AI_N_BF, Average);
DECLARE_TIMER(AI_AI_N_EF, Average);
DECLARE_TIMER(AI_AI_N_UP, Average);
DECLARE_TIMER(AI_AI_N_AP, Average);
DECLARE_TIMER(AI_AI_N_GP, Average);
DECLARE_TIMER(AI_AI_N_MC, Average);
DECLARE_TIMER(AI_AI_N_GC, Average);
DECLARE_TIMER(AI_AI_N_AC, Average);
DECLARE_TIMER(AI_AI_N_A, Average);
DECLARE_TIMER(AI_AI_N_AW, Average);
DECLARE_TIMER(AI_AI_N_F, Average);
DECLARE_TIMER(AI_AI_N_GE, Average);
DECLARE_TIMER(AI_AI_N_FO, Average);
DECLARE_TIMER(AI_AI_N_S, Average);
DECLARE_TIMER(AI_AI_N_FB, Average);
DECLARE_TIMER(AI_AI_N_FS, Average);
DECLARE_TIMER(AI_AI_N_SC, Average);
DECLARE_TIMER(AI_AI_N_P, Average);
DECLARE_TIMER(AI_AI_N_DEF, Average);
DECLARE_TIMER(AI_AI_N_SIG, Average);
DECLARE_TIMER(AI_AI_N_WC, Average);

///////////////////////////////////////
//
//
//

STDMETHODIMP_(void) cAI::NotifySimStart()
{
   AUTO_TIMER(AI_AI_N_SS);

   FOR_EACH_THAT_WANTS(kAICN_SimStart)
   {
      pComponent->OnSimStart();
   }
}

///////////////////////////////////////
//
//
//

STDMETHODIMP_(void) cAI::NotifySimEnd()
{
// @TODO:  Bodisafa 12/8/1999
// This shouldn't be here.
   if (CreatureWeaponObjDestroy)
   {
      CreatureWeaponObjDestroy(GetID());
   }

   AUTO_TIMER(AI_AI_N_SE);
   FOR_EACH_THAT_WANTS(kAICN_SimEnd)
   {
      pComponent->OnSimEnd();
   }
}

///////////////////////////////////////

STDMETHODIMP_(eDamageResult) cAI::NotifyDamage(const sDamageMsg * pMsg)
{
   //
   // Death
   //
   if (pMsg->kind & kDamageMsgTerminate)
   {
      AUTO_TIMER(AI_AI_N_D);
      FOR_EACH_THAT_WANTS(kAICN_Death)
      {
         pComponent->OnDeath(pMsg);
      }
      return kDamageStatusQuo;
   }

   //
   // Notification of "normal" damage
   //
   if ((pMsg->kind & kDamageMsgDamage) &&
        pMsg->data.damage->amount >=0)
   {
      ObjID realCulpritID = GetRealCulprit(pMsg->culprit);
      AUTO_TIMER(AI_AI_N_DAM);
      FOR_EACH_THAT_WANTS(kAICN_Damage)
      {
         pComponent->OnDamage(pMsg, realCulpritID);
      }
   }

   return kDamageNoOpinion;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyWeapon(eWeaponEvent ev, ObjID victim, ObjID culprit)
{
   AUTO_TIMER(AI_AI_N_W);
   FOR_EACH_THAT_WANTS(kAICN_Weapon)
   {
      pComponent->OnWeapon(ev, victim, culprit);
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyBeginFrame()
{
   AUTO_TIMER(AI_AI_N_BF);
   FOR_EACH_THAT_WANTS(kAICN_BeginFrame)
   {
      pComponent->OnBeginFrame();
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyEndFrame()
{
   AUTO_TIMER(AI_AI_N_EF);
   FOR_EACH_THAT_WANTS(kAICN_EndFrame)
   {
      pComponent->OnEndFrame();
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyUpdateProperties(eAIPropUpdateKind kind)
{
   AUTO_TIMER(AI_AI_N_UP);
   FOR_EACH_THAT_WANTS(kAICN_UpdateProps)
   {
      pComponent->OnUpdateProperties(kind);
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(AI_AI_N_AP);
   FOR_EACH_THAT_WANTS(kAICN_ActionProgress)
   {
      pComponent->OnActionProgress(pAction);
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyGoalProgress(const cAIGoal * pGoal)
{
   AUTO_TIMER(AI_AI_N_GP);
   FOR_EACH_THAT_WANTS(kAICN_GoalProgress)
   {
      pComponent->OnGoalProgress(pGoal);
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyModeChange(eAIMode previous, eAIMode mode)
{
   AUTO_TIMER(AI_AI_N_MC);
   // Mode is sent unconditionally
   IAIComponent * pComponent;
   for (int i = 0; i < m_NotifyInfo.Size(); i++)
      if (m_NotifyInfo[i].Wants(kAICN_ModeChange) && (pComponent = m_NotifyInfo[i].pComponent) != NULL)
      {
         pComponent->OnModeChange(previous, mode);
      }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   AUTO_TIMER(AI_AI_N_GC);
   FOR_EACH_THAT_WANTS(kAICN_GoalChange)
   {
      pComponent->OnGoalChange(pPrevious, pGoal);
   }
}

///////////////////////////////////////
//
//
//

void cAI::NotifyActionChange(IAIAction * pPrevious, IAIAction * pAction)
{
   AUTO_TIMER(AI_AI_N_AC);
   FOR_EACH_THAT_WANTS(kAICN_ActionChange)
   {
      pComponent->OnActionChange(pPrevious, pAction);
   }
}

///////////////////////////////////////
//
//
//


void cAI::NotifyAwareness(ObjID source, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   AUTO_TIMER(AI_AI_N_AW);
   FOR_EACH_THAT_WANTS(kAICN_Awareness)
   {
      pComponent->OnAwareness(source, current, pRaw);
   }
}

void cAI::NotifyAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   AUTO_TIMER(AI_AI_N_A);
   FOR_EACH_THAT_WANTS(kAICN_Alertness)
   {
      pComponent->OnAlertness(source, previous, current, pRaw);
   }
}

void cAI::NotifyHighAlert(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   AUTO_TIMER(AI_AI_N_A);
   FOR_EACH_THAT_WANTS(kAICN_HighAlert)
   {
      pComponent->OnHighAlert(source, previous, current, pRaw);
   }
}

void cAI::NotifyFrustration(sAIFrustrated *pData)
{
   AUTO_TIMER(AI_AI_N_F);
   if (pData->pPath)
      pData->pPath->AddRef();

   FOR_EACH_THAT_WANTS(kAICN_Frustration)
   {
      pComponent->OnFrustration(pData->nSourceId, pData->nDestId, pData->pPath);
   }

   if (pData->pPath)
      pData->pPath->Release();
}

///////////////////////////////////////
//
//
//

STDMETHODIMP_(void) cAI::NotifyGameEvent(void * p)
{
   AUTO_TIMER(AI_AI_N_GE);
   FOR_EACH_THAT_WANTS(kAICN_GameEvent)
   {
      pComponent->OnGameEvent(p);
   }
}

////////////////////////////////////
//
// Projectile hook
//

STDMETHODIMP_(void) cAI::NotifyFastObj(ObjID firer, ObjID projectile, const mxs_vector & velocity)
{
   AUTO_TIMER(AI_AI_N_FO);
   FOR_EACH_THAT_WANTS(kAICN_FastObj)
   {
      pComponent->OnFastObj(firer, projectile, velocity);
   }
}

////////////////////////////////////
//
// Sound hook
//

STDMETHODIMP_(void) cAI::NotifySound(const sSoundInfo * pInfo, const sAISoundType * pType)
{
   AUTO_TIMER(AI_AI_N_S);
   if (!m_pSenses || !m_pSenses->Hears(pInfo, pType))
      return;

   if (pType->type)
   {
      FOR_EACH_THAT_WANTS(kAICN_Sound)
      {
         pComponent->OnSound(pInfo, pType);
      }
   }

   if (pType->szSignal[0])
   {
      AIWatch1(Hear, GetID(), "Heard signal %s", pType->szSignal);
      sAISignal signal;
      signal.name = pType->szSignal;
      NotifySignal(&signal);
   }
}

////////////////////////////////////
//
// Tripwire hook
//

STDMETHODIMP_(void) cAI::NotifyTripwire(ObjID objID, BOOL entry)
{
   FOR_EACH_THAT_WANTS(kAICN_Tripwire)
   {
      pComponent->OnTripwire(objID, entry);
   }
}

////////////////////////////////////
//
// Notify of found body
//

STDMETHODIMP_(void) cAI::NotifyFoundBody(ObjID body)
{
   AUTO_TIMER(AI_AI_N_FB);
   FOR_EACH_THAT_WANTS(kAICN_FoundBody)
   {
      pComponent->OnFoundBody(body);
   }
}

////////////////////////////////////
//
// Notify of found something suspicious
//

STDMETHODIMP_(void) cAI::NotifyFoundSuspicious(ObjID suspobj)
{
   AUTO_TIMER(AI_AI_N_FS);
   FOR_EACH_THAT_WANTS(kAICN_FoundSuspicious)
   {
      pComponent->OnFoundSuspicious(suspobj);
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyScriptCommand(eAIScriptCmd command, void * pParms)
{
   AUTO_TIMER(AI_AI_N_SC);
   FOR_EACH_THAT_WANTS(kAICN_ScriptCommand)
   {
      pComponent->OnScriptCommand(command, pParms);
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyProperty(IProperty * pProperty, const sPropertyListenMsg * pMsg)
{
   AUTO_TIMER(AI_AI_N_P);
   FOR_EACH_THAT_WANTS(kAICN_Property)
   {
      pComponent->OnProperty(pProperty, pMsg);
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyDefend(const sAIDefendPoint * pDefendPoint)
{
   AUTO_TIMER(AI_AI_N_DEF);
   FOR_EACH_THAT_WANTS(kAICN_Defend)
   {
      pComponent->OnDefend(pDefendPoint);
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifySignal(const sAISignal * pSignal)
{
   AUTO_TIMER(AI_AI_N_SIG);
   AIWatch1(Signal, GetID(), "Received signal \"%s\"", pSignal->name.operator const char *());
   FOR_EACH_THAT_WANTS(kAICN_Signal)
   {
      pComponent->OnSignal(pSignal);
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyWitnessCombat()
{
   AUTO_TIMER(AI_AI_N_WC);
   FOR_EACH_THAT_WANTS(kAICN_WitnessCombat)
   {
      pComponent->OnWitnessCombat();
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyCollision(const sPhysListenMsg * pMsg)
{
   AUTO_TIMER(AI_AI_N_P);
   FOR_EACH_THAT_WANTS(kAICN_Collision)
   {
      pComponent->OnCollision(pMsg);
   }
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::IsNetworkProxy()
{
   return FALSE;
}

///////////////////////////////////////

void cAI::StartProxyAction(IAIAction *pAction, ulong deltaTime)
{
   CriticalMsg1("Attempted to start proxy action on AI %d when AI is not a proxy", GetID());
}

/////////////////////////////////////////

IAIAction *cAI::GetTheActionOfType(tAIActionType type)
{
   IAIAction *pAction = NULL;
   for (int i = 0; i < m_CurActions.Size(); i++)
   {
      if (m_CurActions[i]->GetType() == type)
         if (pAction == NULL)
            pAction = m_CurActions[i];
         else
            return NULL; // There is more than one action of this type.
   }
   return pAction;
}

/////////////////////////////////////////

void cAI::StopActionOfType(tAIActionType type)
{
   BOOL found = FALSE;
   for (int i = 0; i < m_CurActions.Size();)
   {
      if (m_CurActions[i]->GetType() == type)
      {
         m_CurActions.DeleteItem(i);
         found = TRUE;
      }
      else
         ++i;
   }
}

////////////////////////////////////

STDMETHODIMP_(void) cAI::NotifyGoalsReset(const cAIGoal * pPrevious)
{
   FOR_EACH_THAT_WANTS(kAICN_GoalsReset)
   {
      pComponent->OnGoalsReset(pPrevious);
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAI::SetIsBrainsOnly(BOOL value)
{
   if (value)
      m_state.SetFlags(kAI_IsBrainsOnly);
   else
      m_state.ClearFlags(kAI_IsBrainsOnly);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::PreSave()
{
   FOR_ALL_COMPONENTS()
   {
      if (!pComponent->PreSave())
         return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////

#define kAISubId 0
#define kAIMinor 2

STDMETHODIMP_(BOOL) cAI::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_AI, kAISubId, kAIMinor, pTagFile))
   {
      m_UpdateSchedule.efficiency.Save(pTagFile);
      m_UpdateSchedule.goal.Save(pTagFile);
      m_NullActionTimer.Save(pTagFile);
      m_SimStartTimer.Save(pTagFile);

      AITagMove(pTagFile, &m_flags);

      cAICellArray recent;
      GetRecentCells(&recent);
      AITagMoveDynarray(pTagFile, &recent);

      AICloseTagBlock(pTagFile);
   }

   FOR_ALL_COMPONENTS()
   {
      if (!pComponent->Save(pTagFile))
         return FALSE;
   }

   unsigned nAbilities = m_Abilities.Size();

   if (AIOpenTagBlock(GetID(), kAISL_Goals, kAISubId, kAIMinor, pTagFile))
   {
      BOOL     fIsCurAbility;
      BOOL     fIsCurGoal;
      cStr     abilityNameStr;

      AssertMsg1((!m_pCurAbilityInfo && !m_pCurGoal) ||
                 (m_pCurAbilityInfo && m_pCurGoal && m_pCurGoal->pOwner == m_pCurAbilityInfo->pAbility &&
                 m_pCurGoal == m_pCurAbilityInfo->pGoal),
                 "AI %d Current goal is inconsistent with current ability", GetID());

      AITagMove(pTagFile, &nAbilities);

      for (int i = 0; i < nAbilities; i++)
      {
         abilityNameStr = m_Abilities[i].pAbility->GetName();
         AITagMoveString(pTagFile, &abilityNameStr);

         fIsCurAbility = (m_pCurAbilityInfo == &m_Abilities[i]);
         AITagMove(pTagFile, &fIsCurAbility);
         fIsCurGoal = (m_pCurGoal && m_Abilities[i].pGoal == m_pCurGoal);

         AITagMove(pTagFile, &fIsCurGoal);

         if (!m_Abilities[i].pAbility->SaveGoal(pTagFile, m_Abilities[i].pGoal))
            return FALSE;

         AITagMove(pTagFile, &(m_Abilities[i].modeSuggestion));
      }

      AITagMove(pTagFile, &m_CurGoalLastResult);

      AICloseTagBlock(pTagFile);
   }

   if (AIOpenTagBlock(GetID(), kAISL_Actions, kAISubId, kAIMinor, pTagFile))
   {
      BOOL fHasCurAction = !!m_pCurAbilityInfo && m_CurActions.Size() > 0;
      AITagMove(pTagFile, &fHasCurAction);

      if (fHasCurAction)
      {
         if (!m_pCurAbilityInfo->pAbility->SaveActions(pTagFile, &m_CurActions))
            return FALSE;
      }

      AICloseTagBlock(pTagFile);
   }

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::PostSave()
{
   FOR_ALL_COMPONENTS()
   {
      if (!pComponent->PostSave())
         return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::PreLoad()
{
   FOR_ALL_COMPONENTS()
   {
      if (!pComponent->PreLoad())
         return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::Load(ITagFile * pTagFile)
{
   int i, j;

   if (AIOpenTagBlock(GetID(), kAISL_AI, kAISubId, kAIMinor, pTagFile))
   {
      m_UpdateSchedule.efficiency.Load(pTagFile);
      m_UpdateSchedule.goal.Load(pTagFile);
      m_NullActionTimer.Load(pTagFile);
      m_SimStartTimer.Load(pTagFile);

      AITagMove(pTagFile, &m_flags);

      cAICellArray recent;
      AITagMoveDynarray(pTagFile, &recent);

      AICloseTagBlock(pTagFile);

      for (int i = recent.Size() - 1; i >= 0; i--)
      {
         m_CellStack.Push(&recent[i]);
      }
   }

   FOR_ALL_COMPONENTS()
   {
      if (!pComponent->Load(pTagFile))
         return FALSE;
   }

   if (AIOpenTagBlock(GetID(), kAISL_Goals, kAISubId, kAIMinor, pTagFile))
   {
      unsigned nAbilities;
      BOOL     fIsCurAbility;
      BOOL     fIsCurGoal;
      cStr     loadAbilityName;

      AITagMove(pTagFile, &nAbilities);

      BOOL found;

      for (i = 0; i < nAbilities; i++)
      {
         found = FALSE;
         AITagMoveString(pTagFile, &loadAbilityName);
         for (j = 0; j < m_Abilities.Size(); j++)
         {
            if (loadAbilityName == m_Abilities[j].pAbility->GetName())
            {
               found = TRUE;
               AITagMove(pTagFile, &fIsCurAbility);
               AITagMove(pTagFile, &fIsCurGoal);
               if (!m_Abilities[j].pAbility->LoadGoal(pTagFile, &(m_Abilities[j].pGoal)))
                  goto goals_failed;
               AITagMove(pTagFile, &(m_Abilities[j].modeSuggestion));
               if (m_Abilities[j].pGoal)
               {
                  if (fIsCurAbility && fIsCurGoal)
                  {
                     m_pCurAbilityInfo = &m_Abilities[j];
                     m_pCurGoal = m_Abilities[j].pGoal;
                     m_pCurGoal->AddRef();
                  }
               }
               break;
            }
         }

         if (!found)
         {
            Warning(("Composition of AI %d abilities changed -- discarding goals\n", GetID()));
            goto goals_failed;
         }
      }

      AITagMove(pTagFile, &m_CurGoalLastResult);

goals_failed:

      AICloseTagBlock(pTagFile);

      for (i = 0; i < m_Abilities.Size(); i++)
      {
         if (!m_pCurAbilityInfo || m_Abilities[i].pAbility != m_pCurAbilityInfo->pAbility)
            m_Abilities[i].pAbility->SetControl(FALSE);
         else
            m_Abilities[i].pAbility->SetControl(TRUE);
      }
   }

   AssertMsg1((!m_pCurAbilityInfo && !m_pCurGoal) || (m_pCurAbilityInfo && m_pCurGoal && m_pCurGoal->pOwner == m_pCurAbilityInfo->pAbility && m_pCurGoal == m_pCurAbilityInfo->pGoal), "Inconsistent current goal/current ability on load (%d)", GetID());

#ifndef SHIP
   for (i = 0; i < m_Abilities.Size(); i++)
   {
      if (m_pCurAbilityInfo && m_Abilities[i].pAbility == m_pCurAbilityInfo->pAbility)
      {
         AssertMsg(m_pCurGoal &&
                   m_pCurAbilityInfo->pAbility == m_pCurGoal->pOwner &&
                   m_pCurGoal == m_pCurAbilityInfo->pGoal &&
                   m_pCurAbilityInfo->pAbility->InControl(),
                   "Inconsistent current goal/current ability on load");
      }
      else
      {
         AssertMsg(!m_Abilities[i].pAbility->InControl(),
                   "Inconsistent current goal/current ability on load");
      }
   }
#endif

   if (AIOpenTagBlock(GetID(), kAISL_Actions, kAISubId, kAIMinor, pTagFile))
   {
      cStr loadAbilityName;
      BOOL fHasCurAction;
      AITagMove(pTagFile, &fHasCurAction);

      if (fHasCurAction && m_pCurAbilityInfo)
      {
         m_pCurAbilityInfo->pAbility->LoadActions(pTagFile, &m_CurActions);
      }

      AICloseTagBlock(pTagFile);
   }

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAI::PostLoad()
{
   FOR_ALL_COMPONENTS()
   {
      if (!pComponent->PostLoad())
         return FALSE;
   }

   if (m_pCurAbilityInfo)
   {
      if (m_CurActions.Size() == 0)
         (*m_pCurAbilityInfo->pSignals) |= kAI_SigAct;
   }
   else
   {
      for (int i = 0; i < m_Abilities.Size(); i++)
      {
         *(m_Abilities[i].pSignals) |= (kAI_SigMode | kAI_SigGoal);
      }
   }

   // @HACK: This is an awful hack to make sure that we are never un-ref-ed
   // It's really just in to fix a Shock bug and should be taken out when we no longer
   // need it (hopefully because we have fixed the root cause of the bug). (Jon 8/11/99)
   // @NOTE: Commented out 8/14, since at least one level (station) does
   // intentionally have unref'ed AIs, so we're getting asserts.
   // @NOTE: The hacking continues. Now it is shock specific and checks for a spawn link
   // from any object to the AI. (Jon 8/15/99)

#ifdef SHOCK
      AutoAppIPtr(LinkManager);
      IRelation *pSpawnRelation = pLinkManager->GetRelationNamed("Spawned");

      if ((pSpawnRelation != NULL) && pSpawnRelation->AnyLinks(LINKOBJ_WILDCARD, GetID()))
      {
         AssertMsg1(ObjHasRefs(GetID()), "Found an unrefed, spawned AI: %s", ObjWarnName(GetID()));
#ifdef SHIP
         if (!ObjHasRefs(GetID()))
            ObjSetHasRefs(GetID(), TRUE);
#endif // SHIP
      }
#endif // SHOCK

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
