/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrnew.cpp,v 1.16 2000/03/21 15:53:19 adurant Exp $
//
// AI Combat - new ranged
//

// #define PROFILE_ON 1

#include <aicbrnew.h>

#include <comtools.h>
#include <appagg.h>
#include <config.h>
#include <matrixs.h>
#include <port.h>
#include <rendprop.h>
#include <dmgbase.h>

#include <autolink.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <linkman.h>
#include <traitman.h>
#include <objpos.h>
#include <propbase.h>
#include <iobjsys.h>
#include <objdef.h>
#include <playrobj.h>

#include <contain.h>

#include <phconst.h>
#include <phoprop.h>
#include <phmodobb.h>
#include <phprop.h>
#include <physapi.h>

#include <aiactmrk.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aiactrng.h>
#include <aiapisnd.h>
#include <aiapipth.h>
#include <aiapisns.h>
#include <aisndtyp.h>
#include <aiaware.h>
#include <aiprabil.h>
#include <aiprops.h>
#include <aiprrngd.h>
#include <aiprutil.h>
#include <aidebug.h>
#include <aitagtyp.h>

#include <airngwpn.h>

#include <aitrginf.h>

// modes
#include <aicbrbkp.h>
#include <aicbrclo.h>
#include <aicbrfle.h>
#include <aicbridl.h>
#include <aicbrmpr.h>
#include <aicbrsht.h>
#include <aicbrstl.h>
#include <aicbrstr.h>
#include <aicbrwnd.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

const eAITimerPeriod projRefreshFreq = kAIT_10Hz;

///////////////////////////////////////////////////////
//
// Default physcast callback
//
static BOOL AIDefaultRangedPhyscastCallback(ObjID objID, const cPhysModel* pModel, sAIRangedCombatPhyscastData* pData)
{
   // ignore myself & my target
   if ((objID == (pData)->sourceID) || (objID == (pData)->targetID))
      return FALSE;

   // ignore edge triggers
   if ((pModel->GetType(0) == kPMT_OBB) && ((cPhysOBBModel *)pModel)->IsEdgeTrigger())
      return FALSE;

   // ignore things I can shoot through
   if (GetAIShootsThrough(objID))
      return FALSE;

   // TODO: How do we know when we can shoot through a pore?
   // The projectile has no physics...

   return TRUE;
}

///////////////////////////////////////

BOOL AIInitNewRangedCombatAbility(IAIManager *)
{
   InitRangedRangesProperty();
   InitRangedWoundProperty();
   InitRangedModeProperties();
   return TRUE;
}

///////////////////////////////////////

BOOL AITermNewRangedCombatAbility()
{
   TermRangedRangesProperty();
   TermRangedWoundProperty();
   TermRangedModeProperties();
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINewRangedSubcombat
//

cAINewRangedSubcombat::cAINewRangedSubcombat():
   m_ReactShootTimer(6000, 14000),
   m_physcastCallback(tPhyscastObjTest(AIDefaultRangedPhyscastCallback))
{
}

///////////////////////////////////////

cAINewRangedSubcombat::~cAINewRangedSubcombat()
{
   // @TODO: modes should really be ref-counted
   for (int i=0; i<m_numberModes; i++)
      delete m_pModes[i];
   if (m_pCurrentLocation != NULL)
      delete m_pCurrentLocation;
}

///////////////////////////////////////

void cAINewRangedSubcombat::Reset()
{
   sAIRangedCombatProp *pRangedProp = AIGetRangedCombat(GetID());

   // Init internals
   m_ConsecutiveMisses = 0;

   // Init timers
   m_RefreshProjectileTimer.Set(projRefreshFreq);
   // @TODO: This isn't quite right, since it may cause a reset of the timer
   // every time we switch away and back to the ability. Should probably all
   // be part of a more general mode system.
   m_FiringDelay.Set(kAIT_Min);
   m_FiringDelay.Force();

   // clear modes
   m_currentMode = -1;
   for (int i=0; i<m_numberModes; i++)
      m_pModes[i]->Reset();
   ClearEvents();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINewRangedSubcombat::Init()
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
   m_pModes[m_numberModes++] = new cAIRangedBackup(this);
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

///////////////////////////////////////

STDMETHODIMP_(const char *) cAINewRangedSubcombat::GetName()
{
   return "New ranged combat";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAINewRangedSubcombat::Save(ITagFile * pTagFile)
{
   if (cAISubcombat::Save(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_CombatRanged, 0, 1, pTagFile))
      {
         AITagMove(pTagFile, &m_ConsecutiveMisses);
         m_FiringDelay.Save(pTagFile);
         m_RefreshProjectileTimer.Save(pTagFile);
         m_ReactShootTimer.Save(pTagFile);
         AITagMove(pTagFile, &m_currentMode);

         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAINewRangedSubcombat::Load(ITagFile * pTagFile)
{
   if (cAISubcombat::Load(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_CombatRanged, 0, 1, pTagFile))
      {
         AITagMove(pTagFile, &m_ConsecutiveMisses);
         m_FiringDelay.Load(pTagFile);
         m_RefreshProjectileTimer.Load(pTagFile);
         m_ReactShootTimer.Load(pTagFile);
         AITagMove(pTagFile, &m_currentMode);

         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAINewRangedSubcombat::PostLoad()
{
   cAISubcombat::PostLoad();

   // re-cache properties
   m_pRangedCombatProp = AIGetRangedCombat(GetID());
   m_pRanges = AIGetRangedRanges(GetID());

   Reset();
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINewRangedSubcombat::OnModeChange(eAIMode previous, eAIMode mode)
{
   cAISubcombat::OnModeChange(previous, mode);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINewRangedSubcombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAISubcombat::OnGoalChange(pPrevious, pGoal);
// @TBD (toml 03-04-99): check this
   if (IsOwnerLosingControl(pPrevious, pGoal))
      Reset();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINewRangedSubcombat::OnBeginFrame()
{
   // Update projectiles with how much time has passed
   if (m_RefreshProjectileTimer.Expired())
   {
      m_RefreshProjectileTimer.Reset();
      RefreshProjectilesTime(projRefreshFreq);
   }

   cAISubcombat::OnBeginFrame();
}

///////////////////////////////////////

BOOL cAINewRangedSubcombat::FindEvent(eAINewRangedCombatEvent event)
{
   for (int i=0; i<m_numEvents; i++)
      if (event == m_events[i].m_type)
         return TRUE;
   return FALSE;
}

///////////////////////////////////////

sAIRangedCombatEvent* cAINewRangedSubcombat::GetEvent(eAINewRangedCombatEvent event)
{
   for (int i=0; i<m_numEvents; i++)
      if (event == m_events[i].m_type)
         return &m_events[i];
   return FALSE;
}

///////////////////////////////////////

void cAINewRangedSubcombat::InformOfEvent(eAINewRangedCombatEvent event, BOOL bSignal, void* pData)
{
   if (m_numEvents>kAIRC_MaxEvents)
   {
      // @TODO: deal with this
      Warning(("cAINewRangedSubcombat::InformOfEvent - too many events\n"));
      return;
   }
   m_events[m_numEvents].m_type = event;
   m_events[m_numEvents].m_pData = pData;
   ++m_numEvents;
   if (bSignal)
      SignalAction();
}

///////////////////////////////////////

void cAINewRangedSubcombat::ClearEvents(void)
{
   m_numEvents = 0;
}

///////////////////////////////////////

BOOL cAINewRangedSubcombat::PlaySound(eAISoundConcept CombatSound)
{
   if (!m_pAI->AccessSoundEnactor())
      return FALSE;
   return (m_pAI->AccessSoundEnactor()->RequestConcept(CombatSound));
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINewRangedSubcombat::OnDamage(const sDamageMsg *pMsg, ObjID realCulpritID)
{
   cAISubcombat::OnDamage(pMsg, realCulpritID);
   if (pMsg->kind!=kDamageMsgDamage)
      return;   // if it isnt "damage" damage, go home

   // Sound
   sAIRangedWound* pWoundResponse = AIGetRangedWoundSound(GetID());
   // check propertized thresholds
   if ((int(pMsg->data.damage->amount)>pWoundResponse->m_woundThreshold) &&
      (AIRandom(0, 100)<pWoundResponse->m_responseChance))
   {
      if (int(pMsg->data.damage->amount)<pWoundResponse->m_severeThreshold)
         PlaySound(kAISC_CombatHitDamageLow);
      else
         PlaySound(kAISC_CombatHitDamageHigh);
   }
   else
      PlaySound(kAISC_CombatHitNoDam);

   // Event
   if (InControl())
      InformOfEvent(kAIRC_EventWounded, TRUE, (void*)(pMsg->data.damage->amount));
}

///////////////////////////////////////

DECLARE_TIMER(cAINewRangedSubcombat_OnActionProgress, Average);

STDMETHODIMP_(void) cAINewRangedSubcombat::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAINewRangedSubcombat_OnActionProgress);

   cAISubcombat::OnActionProgress(pAction);

   pAction = pAction->GetTrueAction(); // get our action in case proxied

   if (!IsOwn(pAction))
      return;

   BOOL re_eval = FALSE;

   switch (pAction->GetResult())
   {
      // We've succeeded and are done
      case kAIR_Success:
      {
         switch (pAction->GetType())
         {
            case kAIAT_AttackRanged:
            {
               AIWatch(Ranged, m_pAIState->GetID(), "Completed launch action");
               break;
            }

            case kAIAT_Locomote:
            {
               AIWatch(Ranged, m_pAIState->GetID(), "Arrived at destination");
               break;
            }
         }
         re_eval = TRUE;
         break;
      }

      // Failure!
      case kAIR_Fail:
      {
         if (pAction->GetType() == kAIAT_Locomote)
         {
            AIWatch(Ranged, m_pAIState->GetID(), "Unable to pathfind");
            SignalAction();
         }

         break;
      }

      case kAIR_NoResultSwitch:
      {
         switch (pAction->GetType())
         {
            case kAIAT_Motion:
            case kAIAT_Orient:
               re_eval = FALSE;
               break;
            default:
               re_eval = TRUE;
         }

         break;
      }
   }

   if (re_eval)
      SignalAction();
}

////////////////////////////////////////

int cAINewRangedSubcombat::SelectNewMode(BOOL modesFailed[])
{
   int modeApplicabilities[kAIRC_NumModes];
   int total = 0;
   int selection;

   for (int modeNum = 0; modeNum<m_numberModes; modeNum++)
   {
      if (!modesFailed[modeNum])
         total += m_pModes[modeNum]->SuggestApplicability();
      modeApplicabilities[modeNum] = total;
   }
   if (total == 0)
      if (!modesFailed[0])
         return 0;   // hack - we idle when no one is applicable
      else
         return -1;
   selection = AIRandom(0, total-1);
   for (modeNum = 0; modeNum<m_numberModes; modeNum++)
      if (selection<modeApplicabilities[modeNum])
         break;
   Assert_(modeNum<m_numberModes);
   return modeNum;
}

////////////////////////////////////////

int cAINewRangedSubcombat::SelectInterruptMode(BOOL modesFailed[], eAIRangedModePriority priority)
{
   int modeApplicabilities[kAIRC_NumModes];
   int total = 0;
   int selection;

   for (int modeNum = 0; modeNum<m_numberModes; modeNum++)
   {
      if (!modesFailed[modeNum])
      {
         if (modeNum != m_currentMode)
         {
            if (m_pModes[modeNum]->GetPriority()>=priority)
               total += m_pModes[modeNum]->SuggestInterrupt();
         }
         else
            total += m_pModes[modeNum]->SuggestContinuation();
      }
      modeApplicabilities[modeNum] = total;
   }
   if (total == 0)
      return -1;
   selection = AIRandom(0, total-1);
   for (modeNum = 0; modeNum<m_numberModes; modeNum++)
      if (selection<modeApplicabilities[modeNum])
         break;
   Assert_(modeNum<m_numberModes);
   return modeNum;
}

////////////////////////////////////////

eAIRangedCombatRange cAINewRangedSubcombat::ClassifyRange(float distSq) const
{
   // range class
   if (GetTargetInfo()->distSq<m_rangesSq.m_ranges[0])
      return kAIRC_RangeVeryShort;
   else if (GetTargetInfo()->distSq<m_rangesSq.m_ranges[1])
      return kAIRC_RangeShort;
   else if (GetTargetInfo()->distSq<m_rangesSq.m_ranges[2])
      return kAIRC_RangeIdeal;
   else if (GetTargetInfo()->distSq<m_rangesSq.m_ranges[3])
      return kAIRC_RangeLong;
   else
      return kAIRC_RangeVeryLong;
}

////////////////////////////////////////

cAIMotionAction* cAINewRangedSubcombat::CreateFrustrationAction(void)
{
   cAIMotionAction* pMotionAction = CreateMotionAction();
   pMotionAction->AddTags("Discover 0, Thwarted 0");
   return pMotionAction;
}

////////////////////////////////////////

void cAINewRangedSubcombat::UpdateData(void)
{
   m_pCurrentLocation->SetLocation(*m_pAIState->GetPortalLocation());
   m_pCurrentLocation->TestRangeClass();
   m_pCurrentLocation->TestLOF();   // @TODO: only do this when necessary
   m_pCurrentLocation->Score();
}

////////////////////////////////////////

STDMETHODIMP cAINewRangedSubcombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIAction *pAction = NULL;
   int modeNum;
   BOOL modeFound = FALSE;
   BOOL modesFailed[kAIRC_MaxInstalledModes];
   BOOL interrupt = ((previous.Size()>0) && IsOwn(previous[0]) && previous[0]->InProgress());
   BOOL continuation;
   eAIRangedModePriority interruptPriority;

   // Setup
   UpdateData();
   for (int i=0; i<m_numberModes; i++)
      modesFailed[i] = FALSE;

   // Try to select mode
   if (interrupt)
   {
      Assert_(m_currentMode>=0);
      interruptPriority = m_pModes[m_currentMode]->GetPriority()+1;
      modeNum = SelectInterruptMode(modesFailed, interruptPriority);
      while ((modeNum == -1) && (interruptPriority>0))
      {
         --interruptPriority;
         modeNum = SelectInterruptMode(modesFailed, interruptPriority);
      }
      AIWatch(RangedMode, GetID(), "Interrupt mode select");
   }
   else
   {
      modeNum = SelectNewMode(modesFailed);
      AIWatch(RangedMode, GetID(), "New mode select");
   }

   while (!modeFound && (modeNum != -1))
   {
      if (interrupt && (modeNum == m_currentMode))
         modeFound = m_pModes[modeNum]->CheckContinuation();
      else
         modeFound = m_pModes[modeNum]->CheckPreconditions();
      if (!modeFound)
      {
         AIWatch1(RangedMode, GetID(), "Failed preconditions for mode %s", m_pModes[modeNum]->GetName());
         modesFailed[modeNum] = TRUE;
         if (interrupt)
         {
            modeNum = SelectInterruptMode(modesFailed, interruptPriority);
            while ((modeNum == -1) && (interruptPriority>0))
            {
               --interruptPriority;
               modeNum = SelectInterruptMode(modesFailed, interruptPriority);
            }
         }
         else
            modeNum = SelectNewMode(modesFailed);
      }
      else
         AIWatch1(RangedMode, GetID(), "Passed preconditions for mode %s", m_pModes[modeNum]->GetName());
   }

   continuation = (interrupt && (m_currentMode == modeNum));
   // Start new mode, if any
   if (modeFound)
   {
      if (!continuation)
      {
         if (m_currentMode>=0)
            m_pModes[m_currentMode]->OnLosingControl();
         m_currentMode = modeNum;
         Assert_(m_currentMode>=0);
         m_pModes[m_currentMode]->OnGainingControl();
         AIWatch1(Ranged, GetID(), "Mode changed to %s", m_pModes[m_currentMode]->GetName());
         pAction = m_pModes[m_currentMode]->SuggestAction();
         Assert_(pAction != NULL);
      }
      else // (continuation)
      {
         AIWatch1(Ranged, GetID(), "Continuing mode %s", m_pModes[m_currentMode]->GetName());
         pAction = (cAIAction*)previous[0];
         pAction->AddRef();
      }
   }

   if (pAction != NULL)
      pNew->Append(pAction);
   else
   {
      AIWatch(Ranged, GetID(), "Can't choose mode, going to frustration\n");
      pNew->Append(CreateFrustrationAction());
      SignalFrustration();
   }

   // Clean up
   ClearEvents();

   return S_OK;
}

////////////////////////////////////////

void cAINewRangedSubcombat::RefreshProjectilesTime(int dt)
{
   AutoAppIPtr_(LinkManager, pLinkMan);
   ILinkQuery *query = g_pAIProjectileRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);

   AIProjRelListenerLock = TRUE;

   for (; !query->Done(); query->Next())
   {
      LinkID id = query->ID();

      sAIProjectileRel *pProjData = (sAIProjectileRel *)pLinkMan->GetData(id);
      pProjData->time_since_selected += ((float)dt) / 1000.0;
      pLinkMan->SetData(id, (void *)pProjData);
   }
   SafeRelease(query);

   AIProjRelListenerLock = FALSE;
}

////////////////////////////////////////

void cAINewRangedSubcombat::RefreshProjectilesProj(ObjID projectile)
{
   AutoAppIPtr_(LinkManager, pLinkMan);
   ILinkQuery *query = g_pAIProjectileRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);

   AIProjRelListenerLock = TRUE;

   for (; !query->Done(); query->Next())
   {
      LinkID id = query->ID();
      sLink  link;

      query->Link(&link);

      sAIProjectileRel *pProjData = (sAIProjectileRel *)pLinkMan->GetData(id);

      if (link.dest != projectile)
         pProjData->num_others_selected++;
      else
      {
         pProjData->num_others_selected = 0;
         pProjData->time_since_selected = 0.0;
      }

      pLinkMan->SetData(id, (void *)pProjData);

   }
   SafeRelease(query);

   AIProjRelListenerLock = FALSE;
}

////////////////////////////////////////

struct sSelection
{
   sSelection(ObjID _projectile, sAIProjectileRel *_projectile_data, mxs_vector _targ_loc)
    : projectile(_projectile),
      projectile_data(_projectile_data),
      targ_loc(_targ_loc)
   {};

   ObjID projectile;
   sAIProjectileRel *projectile_data;
   mxs_vector targ_loc;
};

void cAINewRangedSubcombat::SelectProjectile(ObjID *projectile, sAIProjectileRel **proj_link_data,
                                       mxs_vector *targeting_location)
{
   cDynArray<sSelection> selection_list;

   ILinkQuery *query = g_pAIProjectileRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      sAIProjectileRel *pProjData = (sAIProjectileRel *)query->Data();
      sLink link;

      query->Link(&link);

      // Is the projectile available for selection?
      if (!ProjectileAvailable(pProjData))
         continue;

      // Check if the target is hittable
      mxs_vector targ_loc;
      if (ProjectileHittable(pProjData->targeting_method, link.dest, m_pAIState->GetID(),
                              GetTargetInfo()->id, &targ_loc)<0)
         continue;

      sSelection selection(link.dest, pProjData, targ_loc);

      // Bias selection
      for (int i=0; i<pProjData->selection_desire+1; i++)
         selection_list.Append(selection);
   }

   SafeRelease(query);

   if (selection_list.Size() == 0)
   {
      *projectile = OBJ_NULL;
      *proj_link_data = NULL;
   }
   else
   {
      int index = AIRandom(0, selection_list.Size() - 1);

      *projectile = selection_list[index].projectile;
      *proj_link_data = selection_list[index].projectile_data;
      *targeting_location = selection_list[index].targ_loc;
   }
}

////////////////////////////////////////

BOOL cAINewRangedSubcombat::ProjectileAvailable(sAIProjectileRel *proj_link_data)
{
   // Check for constraints
   if (proj_link_data->constraint_type == kCT_NumOthers)
   {
      if (proj_link_data->num_others_selected < proj_link_data->data)
         return FALSE;;
   }
   else
   if (proj_link_data->constraint_type == kCT_NumMisses)
   {
      if (m_ConsecutiveMisses < proj_link_data->data)
         return FALSE;
   }

   // Check for firing delay
   if ((proj_link_data->firing_delay >= 0) && (proj_link_data->firing_delay > proj_link_data->time_since_selected))
      return FALSE;

   return TRUE;
}

////////////////////////////////////////

int cAINewRangedSubcombat::ProjectileHittable(int targetMethod, ObjID projectileID, ObjID sourceID, ObjID targetID,
                                         mxs_vector* pTargetVec) const
{
   Location launchLoc;

   if (sourceID == OBJ_NULL)
      return FALSE;
   // Check for launch offset
   if (AIGetLaunchOffset(sourceID, &launchLoc))
   {
      // check that offset is in world (this is a nasty cut and paste job from code in the launch action)
      Location startLoc;
      Location hitLoc;

      MakeHintedLocationFromVector(&startLoc, &ObjPosGet(m_pAIState->GetID())->loc.vec, &ObjPosGet(m_pAIState->GetID())->loc);
      if (!PortalRaycast(&startLoc, &launchLoc, &hitLoc, 0))
      {
         // we actually need to back up a tiny bit because the hit location
         // is sometimes actually considered "out of world" due to epsilons
         mx_interpolate_vec(&launchLoc.vec, &startLoc.vec, &hitLoc.vec, 0.95);
      }
      return ProjectileHittable(targetMethod, projectileID, launchLoc, targetID, pTargetVec);
   }
   else
      return ProjectileHittable(targetMethod, projectileID, ObjPosGet(sourceID)->loc, targetID, pTargetVec);
}

////////////////////////////////////////

int cAINewRangedSubcombat::ProjectileHittable(int targetMethod, ObjID projectileID, const Location& sourceLoc,
                                         ObjID targetID, mxs_vector* pTargetVec) const
{
   int subModel;

   if (targetID == PlayerObject())
   {
      subModel = PLAYER_HEAD;
      if (!ProjectileHittable(targetMethod, projectileID, sourceLoc, targetID, subModel, pTargetVec))
      {
         subModel = PLAYER_BODY;
         if (!ProjectileHittable(targetMethod, projectileID, sourceLoc, targetID, subModel, pTargetVec))
            subModel = -1;
      }
   }
   else
   {
      subModel = 0;
      if (!ProjectileHittable(targetMethod, projectileID, sourceLoc, targetID, subModel, pTargetVec))
         subModel = -1;
   }
   return subModel;
}

////////////////////////////////////////

BOOL cAINewRangedSubcombat::ProjectileHittable(int targetMethod, ObjID projectileID, const Location& sourceLoc,
                                         ObjID targetID, int targetSubModel, mxs_vector* pTargetVec) const
{
   Location hitLoc;
   Location targetLoc;
   eAIRangedCombatProjTestFlags result = 0;
   mxs_vector targetVec;

   if (targetID == OBJ_NULL)
      return FALSE;

   // Get submodel location
   PhysGetSubModLocation(targetID, targetSubModel, &targetVec);
   MakeHintedLocationFromVector(&targetLoc, &targetVec, &ObjPosGet(targetID)->loc);

   // this is irrelevant unless we re-implement some of the other wacky targetting methods
   *pTargetVec = targetVec;

   switch (targetMethod)
   {
      case kTM_StraightLine:
      {
         ObjID hitObjID;
         sAIRangedCombatPhyscastData data;

         if (m_physcastCallback != NULL)
         {
            data.flags = 0;
            data.sourceID = GetID();
            data.targetID = targetID;
            PhysRaycastSetCallback(m_physcastCallback, (void*)&data);
         }
         BOOL raycastResult = (PhysRaycast((Location)sourceLoc, targetLoc, &hitLoc, &hitObjID, 0.0) == kCollideNone);
         PhysRaycastClearCallback();
         return raycastResult;
      }
      case kTM_Arcing:
      {

         mxs_vector    *pInitVel;
         mxs_vector     midpt;
         Location       midLoc;
         cPhysAttrProp *pAttrProp;
         ObjID          hitObjID;
         mxs_vector offset;
         float zVel0, tMax, zMax;
         float gravity, gravityPct;
         float fXVel;
         float fYVel;
         float f2dVel;
         float fMag;
         mxs_vector Vec2d;

         if (!g_pPhysInitVelProp->Get(projectileID, &pInitVel))
         {
            Warning(("Projectile %d has no initial velocity\n", projectileID));
            return FALSE;
         }

         // This is stupid, but the projectile itself has no physics and the physics attributes
         // property is not inherited, so we just query on the archetype
         AutoAppIPtr(TraitManager);
         if (g_pPhysAttrProp->Get(pTraitManager->GetArchetype(projectileID), &pAttrProp))
            gravityPct = pAttrProp->gravity;
         else
            gravityPct = 100;
         gravity = kGravityAmt*gravityPct/100;

         mx_sub_vec(&offset, pTargetVec, &sourceLoc.vec);
         mx_scale_vec(&Vec2d, &offset, 0.50000);
         Vec2d.z = 0;
         fMag = mx_mag_vec(&Vec2d);

         CalcArc(&offset, pInitVel->x, gravityPct);
         // normalize
         mx_normeq_vec(&offset);
         // scale by initial velocity (assume x component only)
         zVel0 = offset.z*pInitVel->x;

         fXVel = offset.x*pInitVel->x;
         fYVel = offset.y*pInitVel->x;
         f2dVel = sqrt((fXVel * fXVel) + (fYVel * fYVel));

         // Calculate max height

         // wsf: Prviously, this was used to cal midopoint, but we don't always want top of arch.
         // For example, if we're shooting upward at a target, we don't want the point beyond the target.
         //tMax = zVel0/gravity;

         tMax = fMag / f2dVel;

         zMax = sourceLoc.vec.z + zVel0*tMax - 0.5*gravity*tMax*tMax;

         mx_sub_vec(&midpt, &targetVec, &sourceLoc.vec);
         mx_scaleeq_vec(&midpt, 0.5);
         mx_addeq_vec(&midpt, &sourceLoc.vec);
         midpt.z = zMax;

         MakeLocationFromVector(&midLoc, &midpt);
         ComputeCellForLocation(&midLoc);

         if (midLoc.cell == CELL_INVALID)
            return FALSE;

         sAIRangedCombatPhyscastData data;
         if (m_physcastCallback != NULL)
         {
            data.flags = 0;
            data.sourceID = GetID();
            data.targetID = targetID;
            PhysRaycastSetCallback(m_physcastCallback, (void*)&data);
         }

         if (PhysRaycast(midLoc, (Location)sourceLoc, &hitLoc, &hitObjID, 0.0) != kCollideNone)
         {
            PhysRaycastClearCallback();
            return FALSE;
         }

         if (PhysRaycast(midLoc, targetLoc, &hitLoc, &hitObjID, 0.0) != kCollideNone)
         {
            PhysRaycastClearCallback();
            return FALSE;
         }

         PhysRaycastClearCallback();
         return TRUE;
      }

      default:
      {
         Warning(("Unknown targeting method: %d\n", targetMethod));
         return FALSE;
      }
   }
}

////////////////////////////////////////

BOOL cAINewRangedSubcombat::HasTargetLOS()
{
   if (GetTarget() == OBJ_NULL)
      return FALSE;

   const sAIAwareness *pAwareness = m_pAI->GetAwareness(GetTarget());

   return !!(pAwareness->flags & kAIAF_HaveLOS);
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAINewRangedSubcombat::IsMoving(void) const
{
   return ((m_currentMode>=0) && (m_pModes[m_currentMode]->TestFlags(kAIRC_MoveMode)));
}

///////////////////////////////////////////////////////////////////////////////
// This is stupid and won't work with multiple projectiles/AI
//
ObjID cAINewRangedSubcombat::GetDefaultProjectile(void) const
{
   cAutoLinkQuery query(g_pAIProjectileRelation, GetID(), LINKOBJ_WILDCARD);
   if (!query->Done())
      return query.GetDest();
   return OBJ_NULL;
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAINewRangedSubcombat::Pathcast(const mxs_vector& destVec, tAIPathCellID* pEndCell)
{
   return m_pAI->AccessPathfinder()->Pathcast(destVec, pEndCell);
}

