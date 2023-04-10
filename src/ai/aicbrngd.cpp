/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrngd.cpp,v 1.57 2000/02/28 11:31:21 toml Exp $
//
// AI Combat - ranged
//

// #define PROFILE_ON 1

#include <comtools.h>
#include <appagg.h>
#include <config.h>
#include <matrixs.h>
#include <port.h>
#include <rendprop.h>
#include <dmgbase.h>

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

#include <phprop.h>
#include <phoprop.h>
#include <phconst.h>
#include <physapi.h>
#include <physcast.h>

#include <aiactmrk.h>
#include <aiactmov.h>
#include <aiactrng.h>
#include <aiapisnd.h>
#include <aiapipth.h>
#include <aiapisns.h>
#include <aisndtyp.h>
#include <aiaware.h>
#include <aiprabil.h>
#include <aiprrngd.h>
#include <aidebug.h>
#include <aitagtyp.h>

#include <aicbrngd.h>
#include <airngwpn.h>

#include <aitrginf.h>

// for ranged combat change.
#include <aicbrmpr.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

const float kAIRS_HeadingTolerance = 0.25; //copied from aicbrsht.cpp

IRelation * g_pAIRangedWeaponLinks;

static IVectorProperty *g_pAIRangedAudioResponse = NULL;
static sPropertyDesc RangedAudioResp = { "RangedAudioResp", 0, NULL, 0, 0, { AI_ABILITY_CAT, "RangedCombat: Audio Response" }, kPropertyChangeLocally };

static IBoolProperty *g_pAIRangedGruntAlways = NULL;
static sPropertyDesc RangedGruntAlways = { "RngdGruntAlways", 0, NULL, 0, 0, { AI_ABILITY_CAT, "RangedCombat: Grunt Always" }, kPropertyChangeLocally };

static mxs_vector default_audio;


///////////////////////////////////////

BOOL AIInitRangedCombatAbility(IAIManager *)
{
   sRelationDesc     rangedWeaponDesc  = { "AIRangedWeapon", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc noDataDesc        = { "None", 0 };

   // visible
   g_pAIRangedWeaponLinks = CreateStandardRelation(&rangedWeaponDesc, &noDataDesc, (kQCaseSetSourceKnown | kQCaseSetDestKnown));

   g_pAIRangedAudioResponse  = CreateVectorProperty(&RangedAudioResp,kPropertyImplVerySparse);
   g_pAIRangedGruntAlways    = CreateBoolProperty(&RangedGruntAlways,kPropertyImplVerySparse);

   // i think i cant locally static init arrays, so i do them here once
   default_audio.el[0]  = 1.5; default_audio.el[1]  = 7.0; default_audio.el[2]  = 1.5;

   return TRUE;
}

///////////////////////////////////////

BOOL AITermRangedCombatAbility()
{
   SafeRelease(g_pAIRangedAudioResponse);
   SafeRelease(g_pAIRangedWeaponLinks);
   SafeRelease(g_pAIRangedGruntAlways);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIRangedSubcombat
//

cAIRangedSubcombat::cAIRangedSubcombat()
 : m_ReactShootTimer(6000, 14000),
   m_fDamaged(FALSE)
{
}

///////////////////////////////////////

void cAIRangedSubcombat::Reset()
{
   sAIRangedCombatProp *pRangedProp = GetRangedCombatProp();

   // Init internals
   m_ConsecutiveMisses = 0;

   m_CurMarkerPt = OBJ_NULL;
   m_CurDestPt = OBJ_NULL;

   m_MoveStatus = kNoMoveStatus;

   m_DestinationScore = 0.0;
   m_CurDestScore = 0.0;

   m_LocationScore = 0.0;
   m_CurLocScore = 0.0;
   m_CurLocDecayAmt = 1.0;

   m_Attacking = FALSE;
   m_MovingAttacking = FALSE;

   m_BaseTacticalScore = (50.0 - (pRangedProp->cover_desire * 10.0));
   m_TacticalScore = m_BaseTacticalScore;

   // Init bias list
   for (int i=0; i<MAX_POINT_BIASES; i++)
   {
      m_PointBiasList[i].objID = OBJ_NULL;
      m_PointBiasList[i].bias = 0.0;
   }

   // Init timers
   m_RefreshProjectileTimer.Set(PROJ_REFRESH_RATE);
   m_LocationDecayTimer.Set(LOC_DECAY_RATE);
   m_TacticalUpdateTimer.Set(TACTICAL_UPDATE_RATE);
   // @TODO: This isn't quite right, since it may cause a reset of the timer
   // every time we switch away and back to the ability. Should probably all
   // be part of a more general mode system.
   m_FiringDelay.Set(kAIT_Min);
   m_FiringDelay.Force();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIRangedSubcombat::Init()
{
   cAISubcombat::Init();

   SetNotifications(kAICN_BeginFrame |
                    kAICN_ActionProgress |
                    kAICN_Damage |
                    kAICN_GoalChange |
                    kAICN_ModeChange);

   sAIRangedCombatProp *pRangedProp = GetRangedCombatProp();

   if (pRangedProp == NULL)
   {
      g_pRangedCombatProp->Create(m_pAIState->GetID());
      pRangedProp = GetRangedCombatProp();
   }

   AssertMsg1(pRangedProp, "Initializing AI %d, but no ranged combat property?", m_pAIState->GetID());

   if (pRangedProp == NULL)
      return;

   Reset();
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIRangedSubcombat::GetName()
{
   return "Ranged combat";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIRangedSubcombat::Save(ITagFile * pTagFile)
{
   if (cAISubcombat::Save(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_CombatRanged, 0, 1, pTagFile))
      {
         AITagMove(pTagFile, &m_ConsecutiveMisses);
         m_FiringDelay.Save(pTagFile);
         m_RefreshProjectileTimer.Save(pTagFile);
         m_LocationDecayTimer.Save(pTagFile);
         m_TacticalUpdateTimer.Save(pTagFile);
         AITagMove(pTagFile, &m_CurMarkerPt);
         AITagMove(pTagFile, &m_CurDestPt);
         AITagMove(pTagFile, &m_MoveStatus);
         AITagMove(pTagFile, &m_DestinationScore);
         AITagMove(pTagFile, &m_CurDestScore);
         AITagMove(pTagFile, &m_LocationScore);
         AITagMove(pTagFile, &m_CurLocScore);
         AITagMove(pTagFile, &m_CurLocDecayAmt);
         AITagMove(pTagFile, &m_Attacking);
         AITagMove(pTagFile, &m_MovingAttacking);
         AITagMove(pTagFile, &m_TacticalScore);
         AITagMove(pTagFile, &m_BaseTacticalScore);
         AITagMove(pTagFile, &m_PointBiasList);
         AITagMove(pTagFile, &m_fDamaged);
         m_ReactShootTimer.Save(pTagFile);
         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIRangedSubcombat::Load(ITagFile * pTagFile)
{
   if (cAISubcombat::Load(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_CombatRanged, 0, 1, pTagFile))
      {
         AITagMove(pTagFile, &m_ConsecutiveMisses);
         m_FiringDelay.Load(pTagFile);
         m_RefreshProjectileTimer.Load(pTagFile);
         m_LocationDecayTimer.Load(pTagFile);
         m_TacticalUpdateTimer.Load(pTagFile);
         AITagMove(pTagFile, &m_CurMarkerPt);
         AITagMove(pTagFile, &m_CurDestPt);
         AITagMove(pTagFile, &m_MoveStatus);
         AITagMove(pTagFile, &m_DestinationScore);
         AITagMove(pTagFile, &m_CurDestScore);
         AITagMove(pTagFile, &m_LocationScore);
         AITagMove(pTagFile, &m_CurLocScore);
         AITagMove(pTagFile, &m_CurLocDecayAmt);
         AITagMove(pTagFile, &m_Attacking);
         AITagMove(pTagFile, &m_MovingAttacking);
         AITagMove(pTagFile, &m_TacticalScore);
         AITagMove(pTagFile, &m_BaseTacticalScore);
         AITagMove(pTagFile, &m_PointBiasList);
         AITagMove(pTagFile, &m_fDamaged);
         m_ReactShootTimer.Load(pTagFile);

         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIRangedSubcombat::PostLoad()
{
   cAISubcombat::PostLoad();
   Reset();
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIRangedSubcombat::OnModeChange(eAIMode previous, eAIMode mode)
{
   cAISubcombat::OnModeChange(previous, mode);
   Reset();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIRangedSubcombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAISubcombat::OnGoalChange(pPrevious, pGoal);
// @TBD (toml 03-04-99): check this
   if (IsOwnerLosingControl(pPrevious, pGoal))
      Reset();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIRangedSubcombat::OnBeginFrame()
{
   // Update projectiles with how much time has passed
   if (m_RefreshProjectileTimer.Expired())
   {
      m_RefreshProjectileTimer.Reset();
      RefreshProjectilesTime(PROJ_REFRESH_RATE);
   }

   // Decay location score
   if (m_MoveStatus == kAtLoc && m_LocationDecayTimer.Expired())
   {
      m_LocationDecayTimer.Reset();
      DecayLocation();
   }

   // Capacitate toward intial value
   if (m_TacticalUpdateTimer.Expired())
   {
      m_TacticalUpdateTimer.Reset();
      UpdateTacticalScore();
   }

   cAISubcombat::OnBeginFrame();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIRangedSubcombat::OnDamage(const sDamageMsg *pMsg, ObjID realCulpritID)
{
   cAISubcombat::OnDamage(pMsg, realCulpritID);
   if (pMsg->kind!=kDamageMsgDamage)
      return;   // if it isnt "damage" damage, go home

   m_TacticalScore -= pMsg->data.damage->amount * 10;

   if (InControl())
   {
      m_fDamaged = TRUE;
      m_fDamage = pMsg->data.damage->amount;
      SignalAction();
   }

}

///////////////////////////////////////

DECLARE_TIMER(cAIRangedSubcombat_OnActionProgress, Average);

STDMETHODIMP_(void) cAIRangedSubcombat::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIRangedSubcombat_OnActionProgress);

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

               m_Attacking = m_MovingAttacking = FALSE;
               break;
            }

            case kAIAT_Locomote:
            {
               AIWatch(Ranged, m_pAIState->GetID(), "Arrived at destination");

               m_MoveStatus = kAtLoc;

               m_LocationScore = m_DestinationScore;
               m_CurMarkerPt = m_CurDestPt;

               break;
            }
         }
         re_eval = TRUE;

         break;
      }

      // Failure!
      case kAIR_Fail:
      {
         m_Attacking = m_MovingAttacking = FALSE;
         if (pAction->GetType() == kAIAT_Locomote)
         {
            AIWatch1(Ranged, m_pAIState->GetID(), "Unable to pathfind to %d", m_CurDestPt);

            m_CurDestScore = REALLY_BAD;
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
   {
      // Re-evaluate our current target point
      EvalCurDestPt();

      // Evaluate our current location
      EvalCurLocation();

      SignalAction();
   }
}

////////////////////////////////////////

float g_RangedNoFreshenRange = 34.0;
float g_RangedSpeed = 11.0;

////////////////////////////////////////

cAIAction * cAIRangedSubcombat::CheckSuggestDamageAction(const cAIActions & previous)
{
   if (m_fDamaged)
   {
      m_fDamaged = FALSE;

      // @TBD (toml 10-23-98): Um, yes, this is totally random. Just to see how it looks... Hopefully will fix before ship?
      eAISoundConcept concept = kAISC_CoreNum;
      if (m_pAI->AccessSoundEnactor())
      {
         mxs_vector *audio=&default_audio;

         g_pAIRangedAudioResponse->Get(GetID(),&audio);

         float dmg=m_fDamage, var;
         var=((2.0*Rand()-RAND_MAX)/(RAND_MAX+1))*audio->el[2];
         dmg+=var;
         if (dmg>audio->el[0])
         {
            if (dmg>audio->el[1])
               concept = kAISC_CombatHitDamageHigh;
            else
               concept = kAISC_CombatHitDamageLow;
         }
         else
         {
            BOOL grunts=FALSE;
            g_pAIRangedGruntAlways->Get(GetID(),&grunts);
            if (grunts)
               concept = kAISC_CombatHitNoDam;
         }

         m_pAI->AccessSoundEnactor()->RequestConcept(concept);
      }

      if (concept != kAISC_CombatHitNoDam && AIRandom(1, 5) < 5)
      {
         cAIMotionAction * pMotionAction = CreateMotionAction();

         pMotionAction->AddTags("ReceiveWound 0");

         if (concept == kAISC_CombatHitDamageHigh && AIRandom(1, 3) == 1)
            pMotionAction->AddTags("SevereWound 0");

         int direction;
         switch (AIRandom(0, 9))
         {
            case 0:
            case 1:
            case 2:
               direction = 0;
               break;
            case 3:
               direction = 1;
               break;
            case 4:
               direction = 2;
               break;
            case 5:
            case 6:
            case 7:
               direction = 7;
               break;
            case 9:
            case 10:
               direction = 8;
               break;
         }
         if (direction)
            pMotionAction->AddTags(cTag("Direction", direction));

         m_FiringDelay.Set(kAIT_Min);
         m_Attacking = m_MovingAttacking = FALSE;
         return pMotionAction;
      }

   }
   return NULL;
}

////////////////////////////////////////

BOOL cAIRangedSubcombat::CheckReevaluateMove()
{
   if (m_MoveStatus < kAtLoc)
   {
      // Check if our destination's score has dropped significantly
      if ((m_DestinationScore == 0) ||
          (m_CurDestScore < (m_DestinationScore * RE_EVAL_CUR_DEST)))
      {
         if (m_DestinationScore > 0)
         {
            AIWatch(Ranged, m_pAIState->GetID(), "Destination is no longer good enough");

            // @TODO: Add our recently discarded marker to the bias list properly

            m_PointBiasList[0].objID = m_CurDestPt;
            m_PointBiasList[0].bias = -1000.0;
         }
         return TRUE;
      }
   }
   else // At location
   {
      // Check if our curent location has dropped significantly
      if ((m_LocationScore == 0) ||
          (m_CurLocScore < (m_LocationScore * RE_EVAL_CUR_LOC)))
      {
         AIWatch(Ranged, m_pAIState->GetID(), "Current location is no longer good enough");

         m_PointBiasList[0].objID = m_CurMarkerPt;
         m_PointBiasList[0].bias = -1000.0;

         return TRUE;
      }
   }
   return FALSE;
}

////////////////////////////////////////

eAIRCMoveLocKind cAIRangedSubcombat::SelectMarkerMoveLoc(mxs_vector * pLoc)
{
   // Select the best marker point
   sPropertyObjIter iter;

   ObjID best_marker = OBJ_NULL;
   float best_score;

   ObjID cur_marker;
   float cur_score;

   eAIRCMoveLocKind kind = kAIRC_NoMove;

   if (m_TacticalScore > 0)
   {
      AIWatch(Ranged, m_pAIState->GetID(), "Evaluating points (vantage)");

      // Iterate over vantage points
      g_pVantagePtProp->IterStart(&iter);
      while (g_pVantagePtProp->IterNext(&iter, &cur_marker))
      {
         if (OBJ_IS_CONCRETE(cur_marker))
         {
            cur_score = EvaluateLoc(cur_marker, ObjPosGet(cur_marker)->loc.vec);

            if ((best_marker == OBJ_NULL) || (cur_score > best_score))
            {
               best_marker = cur_marker;
               best_score = cur_score;
               kind = kAIRC_Vantage;
            }
         }
      }
   }
   else
   {
      AIWatch(Ranged, m_pAIState->GetID(), "Evaluating points (cover)");

      // Interate over cover points
      g_pCoverPtProp->IterStart(&iter);
      while (g_pCoverPtProp->IterNext(&iter, &cur_marker))
      {
         if (OBJ_IS_CONCRETE(cur_marker))
         {
            cur_score = EvaluateLoc(cur_marker, ObjPosGet(cur_marker)->loc.vec);

            if ((best_marker == OBJ_NULL) || (cur_score > best_score))
            {
               best_marker = cur_marker;
               best_score = cur_score;
               kind = kAIRC_Cover;
            }
         }
      }
   }

   AIWatchTrue(best_score == REALLY_BAD, Ranged, m_pAIState->GetID(), "selected a REALLY BAD marker point");

   if (best_marker && best_score >= 0.0)
   {
      // Good enough marker
      m_CurDestPt = best_marker;
      m_DestinationScore = best_score;

      AIWatch1(Ranged, m_pAIState->GetID(), "Selected point %d", m_CurDestPt);

      // Remove all previous dibs, and dibs on the new marker
      ILinkQuery *query = g_pAIPtDibsRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);
      for (; !query->Done(); query->Next())
         g_pAIPtDibsRelation->Remove(query->ID());
      SafeRelease(query);

      g_pAIPtDibsRelation->Add(m_pAIState->GetID(), best_marker);

      *pLoc = ObjPosGet(best_marker)->loc.vec;
   }
   else
      kind = kAIRC_NoMove;

   return kind;
}

////////////////////////////////////////

eAIRCMoveLocKind cAIRangedSubcombat::SelectMoveLoc(mxs_vector * pLoc)
{
   eAIRCMoveLocKind kind = kAIRC_NoMove;

   m_CurLocDecayAmt = 1.0;

   // Test for a suitable marker in the area
   kind = SelectMarkerMoveLoc(pLoc);

   if (kind)
      return kind; // We found a marker

   // Otherwise, couldn't find a marker or no good markers
   m_CurDestPt = OBJ_NULL;
   m_DestinationScore = 0.0;

   if (!HasTargetLOS())
   {
      AIWatch1(Ranged, m_pAIState->GetID(), "No good enough points, advancing on %d", GetTargetInfo()->id);

      // Advance on target
      *pLoc = GetTargetLoc();
      kind = kAIRC_Target;
   }
   else
   {
      // We have normal AI LOS on target, check if we have a physics line-of-sight
      Location start, end, hit;
      ObjID hit_obj;

      MakeHintedLocationFromVector(&start, &m_pAIState->GetPortalLocation()->vec,
                                           m_pAIState->GetPortalLocation());

      Position *pTargetPos = ObjPosGet(GetTargetInfo()->id);

      BOOL sidestepping = FALSE;

      if (pTargetPos != NULL)
      {
         MakeHintedLocationFromVector(&end, &pTargetPos->loc.vec, &pTargetPos->loc);

         int hit_result = PhysRaycast(start, end, &hit, &hit_obj, 0.0);

         if ((hit_result != kCollideNone) && (hit_obj != GetTargetInfo()->id))
         {
            // can't see target, sidestep
            AIWatch(Ranged, m_pAIState->GetID(), "Object in my way, sidestepping");

            BOOL sidestep_left = (Rand() % 2);

            // Find sidestep location
            mxs_vector to_target;
            mxs_vector up;
            mxs_vector sideways;
            mxs_vector new_loc;

            mx_sub_vec(&to_target, &pTargetPos->loc.vec, m_pAIState->GetLocation());
            mx_unit_vec(&up, 2);

            mx_cross_vec(&sideways, &to_target, &up);
            if (mx_mag2_vec(&sideways) > 0.0001)
               mx_normeq_vec(&sideways);

            const float sidestep_amt= 3;

            if (sidestep_left)
               mx_scaleeq_vec(&sideways, -sidestep_amt);
            else
               mx_scaleeq_vec(&sideways, sidestep_amt);

            mx_add_vec(&new_loc, m_pAIState->GetLocation(), &sideways);

            // See if we can go there
            if (m_pAI->AccessPathfinder()->Pathcast(new_loc))
               sidestepping = TRUE;
            else
            {
               mx_sub_vec(&new_loc, m_pAIState->GetLocation(), &sideways);
               if (m_pAI->AccessPathfinder()->Pathcast(new_loc))
                  sidestepping = TRUE;
            }

            if (sidestepping)
            {
               *pLoc = new_loc;
               kind = kAIRC_Sidestep;
            }
         }

      }
   }
   AIWatchTrue(!kind, Ranged, m_pAIState->GetID(), "Current position is good enough, firing");

   return kind;
}

////////////////////////////////////////

cAIAction * cAIRangedSubcombat::CreateActionForMoveKind(eAIRCMoveLocKind moveKind, const mxs_vector & loc)
{
   if (moveKind)
   {
      eAISpeed moveSpeeds[] =
      {
         kAIS_Stopped,
         kAIS_VeryFast,    // kAIRC_Vantage
         kAIS_VeryFast,    // kAIRC_Cover
         kAIS_VeryFast,    // kAIRC_Target
         kAIS_Normal,      // kAIRC_Sidestep
      };

      cAILocoAction * pLocoAction = CreateLocoAction();
      pLocoAction->Set(loc, moveSpeeds[moveKind]);

      if (moveKind == kAIRC_Target)
      {
         IAISenses * pSenses = m_pAI->AccessSenses();
         BOOL freshen = (pSenses && IsAPlayer(GetTargetInfo()->id));

         if (freshen)
         {
            float distKeepFreshSense = pLocoAction->ComputePathDist()  - g_RangedNoFreshenRange;

            if (distKeepFreshSense > 0.0)
               pSenses->KeepFresh(GetTargetInfo()->id, (distKeepFreshSense / g_RangedSpeed) * 1000.0);
         }

         if (m_pAI->AccessSoundEnactor() && CanPlayReactCharge())
         {
            m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_ReactCharge);
            SetPlayedReactCharge();
         }
      }

      return pLocoAction;
   }
   return NULL;
}

////////////////////////////////////////

cAIAction * cAIRangedSubcombat::CheckSuggestMoveAction(sAIRangedCombatProp * pRangedProp, const cAIActions & previous)
{
   cAIAction * pAction = NULL;

   // Determine if we need to re-do a marker evaluation
   if (!m_Attacking && CheckReevaluateMove())
   {
      mxs_vector       loc;
      eAIRCMoveLocKind moveKind = kAIRC_NoMove;

      moveKind = SelectMoveLoc(&loc);

      pAction = CreateActionForMoveKind(moveKind, loc);

      m_MoveStatus = (pAction) ? kMovingToDest : kAtLoc;
   }
   else if (m_MoveStatus == kMovingToDest)
   {
      if (m_CurLocScore > (m_DestinationScore * CUR_LOC_SCORE_BIAS))
      {
         AIWatch(Ranged, m_pAIState->GetID(), "Cur location much better then destination");

         // Our current location is significantly better than our destination
         m_MoveStatus = kAtLoc;

         m_LocationScore = m_CurLocScore;
         m_CurMarkerPt = OBJ_NULL;
      }
      else
      {
         // Check for fire while moving
         if (pRangedProp->fire_while_moving > 0)
         {
            int max_score = sq(pRangedProp->fire_while_moving);
            int score = AIRandom(0, 50);

            if ((score < max_score) && (m_CurLocScore > 100.0) && m_FiringDelay.Expired())
            {
               AIWatch(Ranged, m_pAIState->GetID(), "Pausing for attack");

               m_MovingAttacking = TRUE;
            }
         }

         // Continue on our way
         if (!m_MovingAttacking && previous.Size() > 0)
         {
            if (((cAIAction *)previous[0])->GetType() != kAIAT_Locomote)
            {
               // Restart our locomotion
               cAILocoAction *pLocoAction = CreateLocoAction();
               if (m_CurDestPt)
                  pLocoAction->Set(ObjPosGet(m_CurDestPt)->loc.vec, kAIS_VeryFast);
               else
                  pLocoAction->Set(ObjPosGet(m_CurDestPt)->loc.vec, kAIS_VeryFast); // not sure what is correct here (toml 03-10-99)

               pAction = pLocoAction;
            }
            else
            {
               // Keep on keepin' on
               pAction = (cAIAction *)previous[0];
               pAction->AddRef();
            }
         }
      }
   }
   return pAction;
}

////////////////////////////////////////

cAIAction * cAIRangedSubcombat::CheckSuggestAttackAction(sAIRangedCombatProp * pRangedProp, const cAIActions & previous)
{
   cAIAction * pAction = NULL;

   if (m_MoveStatus == kAtLoc || m_MovingAttacking)
   {
      // @TODO: check if we should try to duck

      // Continue attacking, if we already are
      if (m_Attacking)
      {
         pAction = (cAIAction *)previous[0];
         pAction->AddRef();
      }
      else if (m_FiringDelay.Expired())
      {
         // Start an attack
         ObjID             projectile;
         sAIProjectileRel *proj_link_data;
         mxs_vector        targeting_location;

         SelectProjectile(&projectile, &proj_link_data, &targeting_location);

         if (projectile != OBJ_NULL)
         {
            AIWatch1(Ranged, m_pAIState->GetID(), "Launching projectile: %d", projectile);

            // Tell all the other projectiles that they weren't selected
            RefreshProjectilesProj(projectile);

            // Pull one out of the inventory
            AutoAppIPtr_(ContainSys, pContainSys);
            AutoAppIPtr_(ObjectSystem, pObjSys);
            ObjID firing_projectile;

            // We want to delete the object if we run out of ammo, but we also
            // want its link around until we're done creating the action, so
            // we defer deletions until then.
            pObjSys->Lock();

            // Clone it
            firing_projectile = pObjSys->Create(projectile, kObjectConcrete);

            // Remove its physics and make it invisible
            g_pPhysTypeProp->Delete(firing_projectile);
            ObjSetHasRefs(firing_projectile, FALSE);

            // Decrement stack count
            int fire_count;
            int burst_count;

            if (proj_link_data->burst_count > 0)
               burst_count = proj_link_data->burst_count;
            else
               burst_count = 1;

            if (proj_link_data->ammo > 0)
            {
               IIntProperty *pStackCountProp = pContainSys->StackCountProp();
               int count;

               if (pStackCountProp->Get(projectile, &count))
               {
                  if (count < burst_count)
                     fire_count = count;
                  else
                     fire_count = burst_count;

                  count -= fire_count;

                  if (count > 0)
                     pStackCountProp->Set(projectile, count);
                  else
                     pObjSys->Destroy(projectile);
               }
               else
                  CriticalMsg("Selected projectile w/o ammo!");
            }
            else
               fire_count = burst_count;

            // @TODO: handle specifying launch point

            cAIAttackRangedAction *pAttackAction = new cAIAttackRangedAction(this);

            if (m_ReactShootTimer.Expired())
            {
               if (m_pAI->AccessSoundEnactor())
                  m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_ReactShoot);
               m_ReactShootTimer.Reset();
            }

            if ((proj_link_data->targeting_method == kTM_StraightLine) ||
                (proj_link_data->targeting_method == kTM_Arcing))
            {
               pAttackAction->Set(GetTargetInfo()->id, firing_projectile, proj_link_data->leads_target,
                                  proj_link_data->accuracy, fire_count, proj_link_data->launch_joint);
            }
            else
            {
               pAttackAction->Set(targeting_location, firing_projectile, proj_link_data->accuracy,
                                  fire_count, proj_link_data->launch_joint);
            }

            pAttackAction->SetRotationParams(AIGetRangedShoot(GetID())->m_rotationSpeed,kAIRS_HeadingTolerance);

            pObjSys->Unlock();

            pAction = pAttackAction;

            m_Attacking = TRUE;

            #define kRandVar 15
            float randVar = 1.0 + (AIRandom(0, kRandVar * 2) - kRandVar) / 100.0;
            m_FiringDelay.Set((eAITimerPeriod)(unsigned)(pRangedProp->firing_delay * 1000 * randVar));
         }
      }
   }

   return pAction;
}


////////////////////////////////////////

cAIAction * cAIRangedSubcombat::CheckSuggestIdleAction(const cAIActions & previous)
{
   Assert_(!m_Attacking);
   AIWatch(Ranged, m_pAIState->GetID(), "Idling");

   // @TODO: now might be a good time to duck

   // Idle
   cAIMoveAction *pMoveAction = CreateMoveAction();
   sAIMoveSuggestion *pSuggestion = new sAIMoveSuggestion;

   pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
   pSuggestion->speed = kAIS_Stopped;
   pSuggestion->facing.type = kAIF_SpecificDir;
   pSuggestion->facing.ang  = m_pAIState->AngleTo(GetTargetLoc()).value;
   pSuggestion->dirArc.SetByCenterAndSpan(pSuggestion->facing.ang, DEGREES(1.0));

   pMoveAction->Add(pSuggestion);

   return pMoveAction;
}

////////////////////////////////////////

STDMETHODIMP cAIRangedSubcombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIAction *pAction = NULL;

   pAction = CheckSuggestDamageAction(previous);

   if (!pAction)
   {
      sAIRangedCombatProp *pRangedProp = pRangedProp = GetRangedCombatProp();

      AssertMsg1(pRangedProp, "Ranged combat AI (%d) w/o ranged combat property!\n", m_pAIState->GetID());

      pAction = CheckSuggestMoveAction(pRangedProp, previous);

      if (!pAction)
      {
         pAction = CheckSuggestAttackAction(pRangedProp, previous);

         if (!pAction)
            pAction = CheckSuggestIdleAction(previous);
      }
   }

   if (pAction != NULL)
      pNew->Append(pAction);

   return S_OK;
}

////////////////////////////////////////

void cAIRangedSubcombat::RefreshProjectilesTime(int dt)
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

void cAIRangedSubcombat::RefreshProjectilesProj(ObjID projectile)
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

void cAIRangedSubcombat::DecayLocation()
{
   float point_decay = 1.0;
   float ai_decay;

   if (m_CurMarkerPt != OBJ_NULL)
   {
      sVantagePtProp *pVantagePt;
      sCoverPtProp   *pCoverPt;

      if (g_pVantagePtProp->Get(m_CurMarkerPt, &pVantagePt))
         point_decay = pVantagePt->decay_speed;
      else
      if (g_pCoverPtProp->Get(m_CurMarkerPt, &pCoverPt))
         point_decay = pCoverPt->decay_speed;
      else
         CriticalMsg2("AI (%d) moving to point (%d) w/o vantage or cover prop!", m_pAIState->GetID(), m_CurMarkerPt);
   }

   sAIRangedCombatProp *pRangedProp;
   if (!g_pRangedCombatProp->Get(m_pAIState->GetID(), &pRangedProp))
   {
      CriticalMsg1("AI (%d) w/o ranged combat property!", m_pAIState->GetID());
      return;
   }

   ai_decay = pRangedProp->decay_speed;

   point_decay = 1.0 - ((1.0 - point_decay) / 100);
   ai_decay = 1.0 - ((1.0 - ai_decay) / 100);

   m_CurLocDecayAmt *= (ai_decay * point_decay);

#if 0
   AIWatch1(Ranged, m_pAIState->GetID(), "Decayed location to %g", m_CurLocDecayAmt);
#endif
}

////////////////////////////////////////

void cAIRangedSubcombat::UpdateTacticalScore()
{
   sAIRangedCombatProp *pRangedProp = GetRangedCombatProp();

   if (pRangedProp)
   {
      mxs_real delta = m_TacticalScore - m_BaseTacticalScore;

      if (delta > 10)
         delta = -0.2;
      else
      if (delta < -10)
         delta = 0.2;
      else
         delta *= -0.2;

      m_TacticalScore += delta;
   }
}

////////////////////////////////////////

void cAIRangedSubcombat::EvalCurDestPt()
{
   if (m_CurDestPt != OBJ_NULL)
      m_CurDestScore = EvaluateLoc(m_CurDestPt, ObjPosGet(m_CurDestPt)->loc.vec);
   else
      m_CurDestScore = REALLY_BAD;
}

////////////////////////////////////////

void cAIRangedSubcombat::EvalCurLocation()
{
   m_CurLocScore = m_CurLocDecayAmt * EvaluateLoc(m_CurMarkerPt, *m_pAIState->GetLocation());
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

void cAIRangedSubcombat::SelectProjectile(ObjID *projectile, sAIProjectileRel **proj_link_data,
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
      if (!ProjectileHittable(pProjData->targeting_method, link.dest, m_pAIState->GetID(),
                              GetTargetInfo()->id, &targ_loc))
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

BOOL cAIRangedSubcombat::ProjectileAvailable(sAIProjectileRel *proj_link_data)
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

BOOL cAIRangedSubcombat::ProjectileHittable(int targ_method, ObjID projectile, ObjID source, ObjID target,
                                         mxs_vector *targeting_location)
{
   if ((target == OBJ_NULL) || (source == OBJ_NULL))
      return FALSE;

   mxs_vector *source_loc = &ObjPosGet(source)->loc.vec;
   const mxs_vector *target_loc = &GetTargetLoc();

   Location start, end, hit;

   switch (targ_method)
   {
      case kTM_StraightLine:
      {
         *targeting_location = *target_loc;

         MakeLocationFromVector(&start, source_loc);
         MakeLocationFromVector(&end, target_loc);

         ComputeCellForLocation(&start);
         if (PortalRaycast(&start, &end, &hit, FALSE))
            return TRUE;
         else
            return FALSE;
      }

      case kTM_Arcing:
      {
         *targeting_location = *target_loc;

         mxs_vector    *pInitVel;
         mxs_real       height;
         mxs_vector     midpt;

         if (!g_pPhysInitVelProp->Get(projectile, &pInitVel))
         {
            Warning(("Projectile %d has no initial velocity\n", projectile));
            return FALSE;
         }

         for (float mult = 0.2; mult <= 1.0; mult += 0.2)
         {
            height = (0.5 * sq(pInitVel->x * mult)) / (2 * kGravityAmt);

            mx_sub_vec(&midpt, target_loc, source_loc);
            mx_scaleeq_vec(&midpt, 0.5);
            midpt.z += height;
            mx_addeq_vec(&midpt, source_loc);

            MakeLocationFromVector(&start, &midpt);
            ComputeCellForLocation(&start);

            if (start.cell == CELL_INVALID)
               continue;

            MakeLocationFromVector(&end, source_loc);

            if (!PortalRaycast(&start, &end, &hit, TRUE))
               continue;

            MakeLocationFromVector(&end, target_loc);

            if (!PortalRaycast(&start, &end, &hit, TRUE))
               continue;

            return TRUE;
         }

         return FALSE;
      }

      case kTM_Reflecting:
      {
         mxs_vector best_target;
         mxs_real   best_score = -1;

         mxs_real   offset_size = 5.0;
         mxs_vector dir;
         mxs_vector targ_loc;
         mxs_vector offset[4];

         // Try casting around the target to find targeting locations
         MakeLocationFromVector(&start, source_loc);
         ComputeCellForLocation(&start);

         mx_sub_vec(&dir, target_loc, source_loc);
         mx_normeq_vec(&dir);

         // Scale offset size
         offset_size *= (100 / mx_dist_vec(target_loc, source_loc));

         // Build "around offset" vectors
         mx_mk_vec(&offset[0], 0, 0,  offset_size);
         mx_mk_vec(&offset[1], 0, 0, -offset_size);

         mx_cross_vec(&offset[2], &dir, &offset[0]);
         mx_normeq_vec(&offset[2]);

         mx_scale_vec(&offset[3], &offset[2], -offset_size);
         mx_scaleeq_vec(&offset[2], offset_size);

         for (int i=0; i<4; i++)
         {
            mx_scale_add_vec(&targ_loc, &offset[i], &dir, 100.0);
            MakeLocationFromVector(&end, &targ_loc);

            if (!PortalRaycast(&start, &end, &hit, FALSE))
            {
               // @HACK: Deal with the fact that the raycaster's hit location
               //        is actually OUT OF THE WORLD arg arg arg
               {
                  mxs_vector vec;

                  mx_sub_vec(&vec, &hit.vec, &start.vec);
                  mx_scaleeq_vec(&vec, 0.99);
                  mx_addeq_vec(&vec, &start.vec);
                  hit.vec = vec;
               }
               targ_loc = hit.vec;

               // See if we can raycast from our collision point to the target
               Location start2;
               MakeLocationFromVector(&start2, &targ_loc);
               MakeLocationFromVector(&end, target_loc);

               ComputeCellForLocation(&start2);
               if (PortalRaycast(&start2, &end, &hit, FALSE))
               {
                  // We can, so score based on distance
                  mxs_real score = mx_dist2_vec(&targ_loc, target_loc);

                  if ((score < best_score) || (best_score < 0))
                  {
                     best_score = score;
                     best_target = targ_loc;
                  }
               }
            }
         }

         if (best_score > 0)
         {
            *targeting_location = best_target;
            return TRUE;
         }
         else
            return FALSE;
      }

      case kTM_Overhead:
      {
         // Cast up from the target to get the targeting location
         MakeLocationFromVector(&start, target_loc);
         MakeLocationFromVector(&end, target_loc);

         end.vec.z += 100;

         ComputeCellForLocation(&start);
         if (!PortalRaycast(&start, &end, &hit, TRUE))
         {
            // @HACK: Deal with the fact that the raycaster's hit location
            //        is actually OUT OF THE WORLD arg arg arg
            {
               mxs_vector vec;

               mx_sub_vec(&vec, &hit.vec, &start.vec);
               mx_scaleeq_vec(&vec, 0.99);
               mx_addeq_vec(&vec, &start.vec);
               hit.vec = vec;
            }

            *targeting_location = hit.vec;

            // Cast from the source to the targeting location
            MakeLocationFromVector(&start, &hit.vec);
            MakeLocationFromVector(&end, source_loc);

            ComputeCellForLocation(&start);
            if (PortalRaycast(&start, &end, &hit, FALSE))
               return TRUE;
            else
               return FALSE;
         }
         else
            return FALSE;
      }

      case kTM_Radius:
      {



      }

      default:
      {
         Warning(("Unknown targeting method: %d\n", targ_method));
         return FALSE;
      }
   }
}

////////////////////////////////////////

BOOL cAIRangedSubcombat::HasTargetLOS()
{
   if (GetTarget() == OBJ_NULL)
      return FALSE;

   const sAIAwareness *pAwareness = m_pAI->GetAwareness(GetTarget());

   return !!(pAwareness->flags & kAIAF_HaveLOS);
}

////////////////////////////////////////

float cAIRangedSubcombat::EvaluateLoc(ObjID markerObj, const mxs_vector &pt_loc)
{
   sAIRangedCombatProp *pAICombatProp = GetRangedCombatProp();
   float score = 50;
   mxs_real ideal_dist;
   mxs_real min_dist;
   mxs_real no_los_bonus;

   ILinkQuery *query;

   // Do we have a target to evaluate against?
   if (GetTarget() == OBJ_NULL)
      return REALLY_BAD;

   // Check for dibs
   query = g_pAIPtDibsRelation->Query(LINKOBJ_WILDCARD, markerObj);
   if (!query->Done())
   {
      sLink link;
      query->Link(&link);

      SafeRelease(query);

      if (link.source != m_pAIState->GetID())
         return REALLY_BAD;
   }
   else
      SafeRelease(query);

   if (m_TacticalScore > 0)
   {
      // Looking for vantage
      if (markerObj != OBJ_NULL)
      {
         sVantagePtProp *pVantagePt;

         if (!g_pVantagePtProp->Get(markerObj, &pVantagePt))
            return REALLY_BAD;

         score = pVantagePt->value;
      }

      ideal_dist = pAICombatProp->ideal_distance;
      min_dist = pAICombatProp->minimum_distance;
      no_los_bonus = -50;
   }
   else
   {
      // Looking for cover
      if (markerObj != OBJ_NULL)
      {
         sCoverPtProp *pCoverPt;

         if (!g_pCoverPtProp->Get(markerObj, &pCoverPt))
            return REALLY_BAD;

         score = pCoverPt->value;
      }

      ideal_dist = pAICombatProp->ideal_distance * 2.0;
      min_dist = pAICombatProp->minimum_distance * 2.0;
      no_los_bonus = 100;
   }

   mxs_vector *ai_loc = (mxs_vector *)m_pAIState->GetLocation();
   const mxs_vector *targ_loc = &GetTargetLoc();

   mxs_real pt_targ_dist_2 = mx_dist2_vec(&pt_loc, targ_loc);

   // Weight based on distances
   if (pt_targ_dist_2 < sq(min_dist))
      return REALLY_BAD;

   score -= sqrt(fabs(sq(ideal_dist) - pt_targ_dist_2));
   score -= mx_dist_vec(&pt_loc, ai_loc);

   // Weight based on LOS
   Location start, end, hit;

   MakeHintedLocationFromVector(&start, &GetTargetLoc(), &ObjPosGet(GetTarget())->loc);
   MakeLocationFromVector(&end, &pt_loc);

   ComputeCellForLocation(&start);
   ObjID hit_obj;
   if ((PhysRaycast(start, end, &hit, &hit_obj, 0.0) != kCollideNone) && (hit_obj != GetTarget()))
      score += no_los_bonus;

   // Score higher if we can hit our target from there
   BOOL hittable = FALSE;

   query = g_pAIProjectileRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);
   for (; !query->Done() && !hittable; query->Next())
   {
      sLink link;
      sAIProjectileRel *pProjData = (sAIProjectileRel *)query->Data();
      mxs_vector targ_loc;

      query->Link(&link);

      if (markerObj != OBJ_NULL)
      {
         if (ProjectileHittable(pProjData->targeting_method, link.dest, markerObj, GetTarget(), &targ_loc))
            hittable = TRUE;
      }
      else
      {
         if (ProjectileHittable(pProjData->targeting_method, link.dest, m_pAIState->GetID(), GetTarget(), &targ_loc))
            hittable = TRUE;
      }
   }
   SafeRelease(query);

   if (hittable)
      score += 150;

   // Fiter by our bias list
   for (int i=0; i<MAX_POINT_BIASES; i++)
   {
      if (m_PointBiasList[i].objID == markerObj)
         score += m_PointBiasList[i].bias;
   }

   return score;
}

////////////////////////////////////////////////////////////////////////////////

sAIRangedCombatProp *cAIRangedSubcombat::GetRangedCombatProp()
{
   if (g_pRangedCombatProp->Get(m_pAIState->GetID(), &m_RangedCombatProp))
      return m_RangedCombatProp;
   else
      return NULL;
}

///////////////////////////////////////////////////////////////////////////////








