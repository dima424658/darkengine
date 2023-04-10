/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimove.cpp,v 1.42 2000/02/16 12:07:13 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <appagg.h>

#include <creatapi.h>
#include <cfgdbg.h>
#include <fastflts.h>
#include <simflags.h>
#include <simstate.h>
#include <simtime.h>

#include <aiapinet.h>
#include <aiactmov.h>
#include <aiactmot.h>
#include <aiactori.h>
#include <aidebug.h>
#include <aidist.h>
#include <aimove.h>
#include <aimovsug.h>

#include <aibasctm.h>
#include <aiprcore.h> // for ground offset
#include <aipthloc.h> // for ground height

#include <creature.h>

// for m_pMotionCoord
#include <mclntapi.h>

// Must be last header
#include <dbmem.h>

#ifndef SHIP
#define ClearDebugPrimitives() \
   { \
      const cAIMoveSuggestions * pDebugSugg;  \
      const sAIMoveGoal * pDebugGoal; \
      \
      m_pAI->DebugGetPrimitiveMove(&pDebugSugg, &pDebugGoal); \
      if (pDebugSugg) \
      { \
         for (int i  = 0; i < pDebugSugg->Size(); i++) \
            delete (*((cAIMoveSuggestions *)pDebugSugg))[i]; \
         delete (cAIMoveSuggestions *)pDebugSugg; \
         delete (sAIMoveGoal *)pDebugGoal; \
         m_pAI->DebugSetPrimitiveMove(NULL, NULL);\
      } \
   }


#define SetDebugPrimitives(pAction) \
   if (pAction && (((cAIAction *)pAction)->GetType() == kAIAT_Move && (g_AIDrawAIs && (g_AIDebugDrawMoveGoal || g_AIDebugDrawSuggs)))) \
   { \
      cAIMoveSuggestions * pDebugSugg;  \
      sAIMoveGoal * pDebugGoal; \
      pDebugSugg = new cAIMoveSuggestions; \
      pDebugSugg->Add(((cAIMoveAction *)pAction)->GetSuggestions()); \
      AddMovementRegulations(*pDebugSugg); \
      for (int i = 0; i < pDebugSugg->Size(); i++) \
         (*pDebugSugg)[i] = new sAIMoveSuggestion(*((*pDebugSugg)[i])); \
      pDebugGoal = new sAIMoveGoal(((cAIMoveAction *)pAction)->GetMoveGoal());  \
      m_pAI->DebugSetPrimitiveMove(pDebugSugg, pDebugGoal); \
   }

#else
#define ClearDebugPrimitives()
#define SetDebugPrimitives(pAction)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveEnactor
//

cAIMoveEnactor::cAIMoveEnactor()
 : m_flags(0),
   m_pMotionCoord(NULL),
   m_StandTimer(4000, 10000),
   m_StandCount(0),
   m_ImpulsePeriod(kAIT_10Hz), // 10 per sec max
   m_pTargetVel(new sAIImpulse)
{
   memset(m_pTargetVel, 0, sizeof(*m_pTargetVel));
}

///////////////////////////////////////

cAIMoveEnactor::~cAIMoveEnactor()
{
   ClearDebugPrimitives();
   MotCleanup();
   delete m_pTargetVel;
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIMoveEnactor::GetName()
{
   return "Standard Move Enactor";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIMoveEnactor::Init()
{
   cAIMoveEnactorBase::Init();
   SetNotifications(kAICN_ModeChange | kAICN_SimStart);
   SetMotor(CreatureGetMotorInterface(GetID())); // this is null if not a creature
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMoveEnactor::GetTargetVel(sAIImpulse * pResult)
{
   memcpy(pResult, m_pTargetVel, sizeof(sAIImpulse));
   return TRUE;
}

///////////////////////////////////////

#define kMoveEnVer 2

STDMETHODIMP_(BOOL) cAIMoveEnactor::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_MoveEnactor, 0, kMoveEnVer, pTagFile))
   {
      SaveNotifications(pTagFile);
      MotSave(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMoveEnactor::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_MoveEnactor, 0, kMoveEnVer, pTagFile))
   {
      LoadNotifications(pTagFile);
      MotLoad(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIMoveEnactor::OnModeChange(eAIMode previous, eAIMode mode)
{
   if (mode < kAIM_Efficient)
   {
      if (!m_pAIState->IsCreature())
         SetObjImpulse(GetID(), 0, 0, 0, 0);
      else
      {
         cCreature* pCreature = CreatureFromObj(GetID());
         Assert_(pCreature!=NULL);
         if (pCreature->IsPhysical())
            SetObjImpulse(GetID(), 0, 0, 0, 0);
      }
   }

   if (!m_pMotionCoord)
      return;

   MotOnModeChange(previous, mode);
}

///////////////////////////////////////
//
// Add a movement regulator
//

STDMETHODIMP cAIMoveEnactor::InstallRegulator(IAIMoveRegulator * pRegulator)
{
   m_MoveRegulators.Append(pRegulator);
   return S_OK;
}

///////////////////////////////////////
//
//
//

STDMETHODIMP_(eAIMEStatus) cAIMoveEnactor::GetStatus()
{
   eAIMEStatus status;

   if (m_pMotionCoord && m_pAIState->GetMode() != kAIM_Efficient)
      status = MotStatus();
   else
      status = NonMotStatus();

   if ((status == kAIME_ActiveBusy) && m_pMotionCoord)
   {
      for (int i = 0; i < m_MoveRegulators.Size(); i++)
      {
         if (m_MoveRegulators[i]->WantsInterrupt())
            {
            // wsf: added to fix new T2 bug where AI's wait for "standing" mocap to finish (5 secs)
            // before entering opened door it opened. This call sets flag for this frame only.
            m_pMotionCoord->NotifyThatCanInterrupt();
            return kAIME_ActiveInterrupt;
            }
      }
   }
   return status;
}

///////////////////////////////////////

STDMETHODIMP cAIMoveEnactor::NoAction(ulong deltaTime)
{
   ClearDebugPrimitives();

   if (m_pMotionCoord && m_pAIState->GetMode() != kAIM_Efficient)
      return MotNoAction(deltaTime);
   return NonMotNoAction(deltaTime);
}

///////////////////////////////////////
//
//
//

STDMETHODIMP cAIMoveEnactor::EnactAction(cAIMoveAction * pAction, ulong deltaTime)
{
   ClearDebugPrimitives();
   SetDebugPrimitives(pAction);

   AssertMsg2(pAction->Access()->tags.Size() < 32, "AI %d made a move request with an unusually high number of tags (%d)", GetID(), pAction->Access()->tags.Size());

   if (m_pMotionCoord && m_pAIState->GetMode() != kAIM_Efficient)
      return MotEnact(pAction, deltaTime);
   return NonMotEnact(pAction, deltaTime);
}

///////////////////////////////////////
//
//
//

STDMETHODIMP cAIMoveEnactor::EnactAction(cAIMotionAction * pAction, ulong deltaTime)
{
   ClearDebugPrimitives();

   AssertMsg2(pAction->Access()->tags.Size() < 32, "AI %d made a motion request with an unusually high number of tags (%d)", GetID(), pAction->Access()->tags.Size());

   if (m_pMotionCoord && m_pAIState->GetMode() != kAIM_Efficient)
      return MotEnact(pAction, deltaTime);
   return NonMotEnact(pAction, deltaTime);
}

///////////////////////////////////////
//
//
//

STDMETHODIMP cAIMoveEnactor::EnactAction(cAIOrientAction * pAction, ulong deltaTime)
{
   ClearDebugPrimitives();

   AssertMsg2(pAction->Access()->tags.Size() < 32, "AI %d made an orient request with an unusually high number of tags (%d)", GetID(), pAction->Access()->tags.Size());

   if (m_pMotionCoord && m_pAIState->GetMode() != kAIM_Efficient)
      return MotEnact(pAction, deltaTime);
   return NonMotEnact(pAction, deltaTime);
}

///////////////////////////////////////
//
//
//

STDMETHODIMP_(BOOL) cAIMoveEnactor::NewRegulations()
{
   if (m_pAIState->GetMode() != kAIM_Efficient)
   {
      for (int i = 0; i < m_MoveRegulators.Size(); i++)
      {
         if (m_MoveRegulators[i]->NewRegulations())
            return TRUE;
      }
      return FALSE;
   }
   return TRUE;
}

///////////////////////////////////////
//
//
//


DECLARE_TIMER(cAIMoveEnactor_AddMovementRegulations, Average);

STDMETHODIMP cAIMoveEnactor::AddMovementRegulations(cAIMoveSuggestions & suggestions)
{
   AUTO_TIMER(cAIMoveEnactor_AddMovementRegulations);

   HRESULT result = S_FALSE;
   if (m_pAIState->GetMode() >= kAIM_Normal)
   {
      for (int i = 0; i < m_MoveRegulators.Size(); i++)
      {
         if (m_MoveRegulators[i]->SuggestRegulations(suggestions) == S_OK)
            result = S_OK;
      }
   }
   return result;
}

///////////////////////////////////////
//
// ResolveSuggestions
//
// ResolveSuggestions is the heart of the low-level spatial navigation distributed
// process which is base Movement Enactor and the Movement Regulators.
//
// ResolveSuggestions looks at all of the suggestions supplied by the evaluators
// this frame and combines (or chooses among them) to make the decision (and pick
// the AI's low-level move goal, if any).
//
// The suggestions take the form of an movement direction arc, which is
// described by a goal angle and a latitude of degress around that goal angle for
// which the suggestion is still viable.  The suggestions then have all sorts
// of extra data which explains various restrictions upon the suggestion
// such as movement speed and direction faced during the movement.
//
// The ResolveSuggestions works to find the best goal from the various
// sugegstions by following this process:
//
// o Each suggestion is turned into two vectors (called real vectors),
//   where each vector represents one of the "posts" or ends of each suggestions
//   movement direction arc.
//
// o In the middle of each two adjacent real vectors is a solution vector.
//
// o Each solution vector has a total bias which is equal to the
//   the highest bias of the suggestions which cross the solution vector
//   PLUS all of the biases of the other suggestions which cross the
//   solution vector and can be combined with the highest bias suggestion.
//
// o The highest solution vector is then chosen as the direction of movement.
//
// o The other data in the AI's movement goal is derived from the
//   the highest bias suggestion which crosses the solution vector.
//

///////////////////

#define kMaxSolutionVectors 60
#define kSolutionVecIncrement (TWO_PI / kMaxSolutionVectors)

struct sAISolutionVector
{
   int                       bias;
   int                       highestBias;
   const sAIMoveSuggestion * pHighestBiasSuggestion;
};

inline void AddBiasedSuggestion(sAISolutionVector * solutionVectors, int i, int bias, const sAIMoveSuggestion * pSuggestion)
{
   solutionVectors[i].bias += bias;
   if (bias > solutionVectors[i].highestBias)
   {
      solutionVectors[i].highestBias = bias;
      solutionVectors[i].pHighestBiasSuggestion = pSuggestion;
   }
}

///////////////////

DECLARE_TIMER(AI_ME_RS, Average);

STDMETHODIMP cAIMoveEnactor::ResolveSuggestions(const cAIMoveSuggestions & suggestions,
                                                const sAIMoveSuggestion ** ppBestSuggestion,
                                                sAIMoveGoal * pResult)
{
   AUTO_TIMER(AI_ME_RS);

   if (!suggestions.Size())
      return S_FALSE;

   int iSuggestion, i, ccw, cw, center, bias;

   const sAIMoveSuggestion * pSuggestion;

   static sAISolutionVector solutionVectors[kMaxSolutionVectors];
   static floatang          solutionDirs[kMaxSolutionVectors];
   static BOOL              fInit;

   ////////////////////////////////////
   //
   // Initialize the solution vectors
   //

   if (!fInit)
   {
      fInit = TRUE;
      for (i = 0; i < kMaxSolutionVectors; i++)
         solutionDirs[i].value = TWO_PI * i / kMaxSolutionVectors;
   }

   memset(solutionVectors,0,sizeof(solutionVectors));

   ////////////////////////////////////
   //
   // Add the suggestions into the solution vector
   // And keep track of which suggestion was the
   // "best contributor" to that solution vector.
   //

   for (iSuggestion = 0; iSuggestion < suggestions.Size(); iSuggestion++)
   {
      pSuggestion = suggestions[iSuggestion];

      ccw    = kMaxSolutionVectors * pSuggestion->dirArc.ccw.value / TWO_PI;
      cw     = kMaxSolutionVectors * pSuggestion->dirArc.cw.value / TWO_PI;
      center = kMaxSolutionVectors * pSuggestion->dirArc.center().value / TWO_PI;

      if (cw <= ccw)
      {
         for (i = cw; i <= ccw; i++)
         {
            if (pSuggestion->bias > 0)
            {
               bias = pSuggestion->bias - abs(center - i);
               if (bias < 1) bias = 1;
            }
            else
               bias = pSuggestion->bias;

            AddBiasedSuggestion(solutionVectors, i, bias, pSuggestion);
         }
      }
      else
      {
         for (i = cw; i < kMaxSolutionVectors; i++)
         {
            if (pSuggestion->bias > 0)
            {
               if (center < ccw)
                  bias = pSuggestion->bias - (kMaxSolutionVectors - i) - center;
               else
                  bias = pSuggestion->bias - abs(center - i);
               if (bias < 1) bias = 1;
            }
            else
               bias = pSuggestion->bias;

            AddBiasedSuggestion(solutionVectors, i, bias, pSuggestion);
         }

         for (i = 0; i <= ccw; i++)
         {
            if (pSuggestion->bias > 0)
            {
               if (center < ccw)
                  bias = pSuggestion->bias - abs(center - i);
               else
                  bias = pSuggestion->bias - (kMaxSolutionVectors - center) - i;
               if (bias < 1) bias = 1;
            }
            else
               bias = pSuggestion->bias;

            AddBiasedSuggestion(solutionVectors, i, bias, pSuggestion);
         }
      }
   }

   ////////////////////////////////////
   //
   // Find the best solution
   //

   int   best = -1, highestBias = 0;

   for (i = 0; i < kMaxSolutionVectors; i++)
   {
      if (solutionVectors[i].bias > highestBias)
      {
         highestBias = solutionVectors[i].bias;
         best = i;
      }
   }

   ////////////////////////////////////
   //
   // Update the move goal
   // @TBD (toml 03-30-98): facing etc should probably only be derived from input suggestions, not regulations
   //
   if (best < 0)
      return S_FALSE;

   pSuggestion = solutionVectors[best].pHighestBiasSuggestion;

   if (ppBestSuggestion)
      *ppBestSuggestion = pSuggestion;


   pResult->dir = solutionDirs[best];
   floatang centerHighest = solutionVectors[best].pHighestBiasSuggestion->dirArc.center();
   floatang diffSolution = (centerHighest - pResult->dir);

   // if close enough to best suggesting, just go with it to eliminate rounding-up errors (toml 10-15-98)
   if (diffSolution.value < kSolutionVecIncrement || diffSolution.value > TWO_PI - kSolutionVecIncrement)
   {
      pResult->dir = centerHighest;
   }

   pResult->dest   = pSuggestion->dest;
   pResult->facing = pSuggestion->facing;
   pResult->speed  = pSuggestion->speed;

   return S_OK;
}


///////////////////////////////////////

floatang cAIMoveEnactor::ComputeFacing(const sAIMoveGoal & goal)
{
   const cMxsVector & loc = *m_pAIState->GetLocation();
   eAIFacing          facingType;

   floatang result = 0;

   if (goal.facing.type == kAIF_Any)
   {
#if 0
// @TBD (toml 04-13-98): if (have combat target) face it ?
#endif
      facingType = kAIF_MoveDir;
   }
   else
      facingType = goal.facing.type;

   switch (facingType)
   {
      case kAIF_MoveDir:
         result = goal.dir;
         break;

      case kAIF_Dest:
         result.set(loc.x, loc.y, goal.dest.x, goal.dest.y);
         break;

      case kAIF_AltPos:
         result.set(loc.x, loc.y, goal.facing.pos.x, goal.facing.pos.y);
         break;

      case kAIF_SpecificDir:
         result = goal.facing.ang;
         break;
   }
   return result;
}

///////////////////////////////////////
//
//
//

eAIMEStatus cAIMoveEnactor::NonMotStatus()
{
   if (m_ImpulsePeriod.Expired())
      return kAIME_ActiveInterrupt;
   return kAIME_ActiveBusy;
}

///////////////////////////////////////

HRESULT cAIMoveEnactor::NonMotNoAction(ulong deltaTime)
{
   if (m_pAIState->GetMode() < kAIM_Efficient ||
      !m_ImpulsePeriod.Expired())
      return S_OK;

   if (m_pAIState->IsCreature())
   {
      // Don't try to update physics on non-physical, efficient creatures
      cCreature* pCreature = CreatureFromObj(GetID());
      Assert_(pCreature);
      if (!pCreature->IsPhysical())
         return S_OK;
   }

   // Run 1/4 as often
   if ((m_flags & kLastWasNoAction) && (m_StandCount % 4))
   {
      m_StandCount++;
      return S_OK;
   }
   m_StandCount++;

   sAIMoveGoal nullGoal;

   nullGoal.dir = m_pAIState->GetFacingAng();
   nullGoal.dest = *m_pAIState->GetLocation();

   m_flags |= kLastWasNoAction;

   return NonMotEnactMoveGoal(nullGoal, deltaTime);
}

///////////////////////////////////////
//
// Crude wedge movement
//

HRESULT cAIMoveEnactor::NonMotEnact(cAIMoveAction * pAction, ulong deltaTime)
{
   if (!m_ImpulsePeriod.Expired())
      return S_FALSE;

   m_ImpulsePeriod.Reset();

   m_flags &= ~kLastWasNoAction;
   m_StandCount = 0;

   return NonMotEnactMoveGoal(pAction->GetMoveGoal(), deltaTime);
}

///////////////////////////////////////
//
//
//

HRESULT cAIMoveEnactor::NonMotEnact(cAIMotionAction * pAction, ulong deltaTime)
{
   m_flags &= ~kLastWasNoAction;
   m_StandCount = 0;

   return E_FAIL;
}

///////////////////////////////////////
//
//
//

HRESULT cAIMoveEnactor::NonMotEnact(cAIOrientAction * pAction, ulong deltaTime)
{
   m_flags &= ~kLastWasNoAction;
   m_StandCount = 0;
   // @TBD
   return E_FAIL;
}

///////////////////////////////////////

// im a total moron, but for now, this seems the easiest way to get my stuff going....
void cAIMoveEnactor::CalculateTargetVel(const sAIMoveGoal & goal, ulong deltaTime, sAIImpulse * pResult)
{
   const cMxsVector &  loc = *m_pAIState->GetLocation();
   const tAIPathCellID cell = m_pAIState->GetPathCell();

   static float goalSpeedVal[kAIS_Num] = { 0.0, 0.10, 0.20, 0.30, 0.65, 1.00 };

   mxs_vector dir;

   mx_sub_vec(&dir, &goal.dest, m_pAIState->GetLocation());
   if (mx_mag2_vec(&dir) > 0.0001)
      mx_normeq_vec(&dir);

#ifdef WHAT_IS_THIS_MESS
   float               desiredZ;

   if (cell)
      desiredZ = AIGetZAtXYOnCell(loc,cell) + m_pAI->GetGroundOffset();
   else
      desiredZ = loc.z;

   mprintf("dirz %g desz %g at %g...",dir.z,desiredZ,m_pAIState->GetLocation()->z);

   if (ffabsf(m_pAIState->GetLocation()->z-desiredZ) < 0.6)
      dir.z = desiredZ-m_pAIState->GetLocation()->z;
   else if (m_pAIState->GetLocation()->z > desiredZ)
      dir.z = -0.6;
   else        // (loc.z <= desiredZ - 0.6)
      dir.z = +0.6;

   mprintf("ffabs got dirz %g\n",dir.z);

   dir.z=0;
#endif

   // i question AIGetMaxSpeed, i must admit...
   mx_scaleeq_vec(&dir, AIGetMaxSpeed(GetID()) * goalSpeedVal[goal.speed] * 2.0);

   pResult->vec = dir;
   pResult->facing = 0;
   pResult->flags &= ~kAIIF_Rotating;
}

void cAIMoveEnactor::CalculateImpulse(const sAIMoveGoal & goal, ulong deltaTime, sAIImpulse * pResult)
{
   const cMxsVector &  loc = *m_pAIState->GetLocation();
   const tAIPathCellID cell = m_pAIState->GetPathCell();
   mxs_vector          vel;
   float               x, y, z, newFacing, speed, maxSpeed, distanceFromGoal;
   float               desiredZ;
   floatang            currentFacing, facingRelMoveDir, delta;
   floatang            goalFacing, moveDir, dirDiff;
   float               maxTurn;

   static float goalSpeedVal[kAIS_Num] =
   {
      0.0,
      0.10,
      0.20,
      0.30,
      0.65,
      1.00
   };

#ifdef ATTEMPT_TO_BE_RIGHT
   GetObjVelocity(GetID(), &vel);
   float mag2_vel=mx_mag2_vec(&vel);

   if ((m_pAIState->GetMode() > kAIM_Efficient)&&(mag2_vel>0.04)) {}
#endif
   if (m_pAIState->GetMode() > kAIM_Efficient)
   {
      currentFacing = m_pAIState->GetFacingAng();

      // @TBD: THIS CODE IS WRONG - if vel is small, this blows up, you get bogus data!
      GetObjVelocity(GetID(), &vel); // this should get pulled, and we should do ATTEMPT_TO_BE_RIGHT, above
      moveDir.set(0, 0, vel.x, vel.y);

      dirDiff = floatang_smallestDeltaOf2Angs(moveDir, goal.dir);

      maxSpeed = AIGetMaxSpeed(GetID());

      if (dirDiff < PI)
      {
         // if we're already heading, roughly, in the right direction let's
         // start slowing ourselves down so we don't run past our goal.
         distanceFromGoal = AIDistanceSq(loc, goal.dest);
         if (distanceFromGoal > sq(maxSpeed))
            speed = maxSpeed;
         else if (distanceFromGoal < sq(0.2))
            speed = maxSpeed * 0.3;
         else if (distanceFromGoal < sq(0.6))
            speed = maxSpeed * 0.7;
         else // > 0.6  < maxspeed
            speed = maxSpeed * 0.5;
      }
      else
         speed = maxSpeed;

      AssertMsg(goal.speed >= kAIS_Stopped && goal.speed < kAIS_Num, "Invalid speed");

      speed *= goalSpeedVal[goal.speed];

      if (SimStateCheckFlags(kSimPhysics))
      {
         facingRelMoveDir = goal.dir - currentFacing;
         x = cos(facingRelMoveDir.value) * speed;
         y = sin(facingRelMoveDir.value) * speed;
      }
      else
      {
         x = cos(goal.dir.value) * speed;
         y = sin(goal.dir.value) * speed;
      }

      if (cell)
         desiredZ = AIGetZAtXYOnCell(loc,cell) + m_pAI->GetGroundOffset();
      else
         desiredZ = loc.z;

      if (ffabsf(loc.z-desiredZ) < 0.6)
         z = desiredZ-loc.z;
      else if (loc.z > desiredZ)
         z = -0.6;
      else        // (loc.z <= desiredZ - 0.6)
         z = +0.6;

      goalFacing = ComputeFacing(goal);

      delta = goalFacing - currentFacing;
      if (delta.value <= PI)
         newFacing = delta.value;
      else
         newFacing = delta.value - 2 * PI;

      float turnScale;

      #define kTurnSlowAng DEGREES(60)

      turnScale = (kTurnSlowAng - fabs(newFacing));
      if (turnScale < 0)
      {
         // Big turn -- slow down by 1/3
         x = x / 3;
         y = y / 3;
         z = z / 3;
      }
      else if (turnScale < kTurnSlowAng)
      {
         // Else, slow down by factor of 1/3 to 1
         x = x * (0.33 + (turnScale / kTurnSlowAng) * 0.63);
         y = y * (0.33 + (turnScale / kTurnSlowAng) * 0.63);
         z = z * (0.33 + (turnScale / kTurnSlowAng) * 0.63);
      }

      // Constrain turn rate
      maxTurn = AIGetTurnRate(GetID())*deltaTime;
      if (fabs(newFacing) > maxTurn)
      {
         if (newFacing < 0)
            newFacing = -maxTurn;
         else
            newFacing = maxTurn;
      }

      pResult->vec.x = x;
      pResult->vec.y = y;
      pResult->vec.z = z;
      pResult->facing = newFacing;
      pResult->flags |= kAIIF_Rotating;
   }
   else
   {
      mxs_vector dir;

      mx_sub_vec(&dir, &goal.dest, m_pAIState->GetLocation());
      if (mx_mag2_vec(&dir) > 0.0001)
         mx_normeq_vec(&dir);
      newFacing = 0;

      if (cell)
         desiredZ = AIGetZAtXYOnCell(loc,cell) + m_pAI->GetGroundOffset();
      else
         desiredZ = loc.z;

      if (ffabsf(m_pAIState->GetLocation()->z-desiredZ) < 0.6)
         dir.z = desiredZ-m_pAIState->GetLocation()->z;
      else if (m_pAIState->GetLocation()->z > desiredZ)
         dir.z = -0.6;
      else        // (loc.z <= desiredZ - 0.6)
         dir.z = +0.6;

      mx_scaleeq_vec(&dir, AIGetMaxSpeed(GetID()) * goalSpeedVal[goal.speed]);

      pResult->vec = dir;
      pResult->facing = newFacing;
      pResult->flags &= ~kAIIF_Rotating;
   }
   //#define LOUD_IMPULSE
#ifdef LOUD_IMPULSE
   mprintf("CalcImp dt %d st %d, got %g %g %g, at %g %g %g\n",
           deltaTime, GetSimTime(),
           pResult->vec.x,pResult->vec.y,pResult->vec.z,
           m_pAIState->GetLocation()->x,
           m_pAIState->GetLocation()->y,
           m_pAIState->GetLocation()->z);
#endif
}

///////////////////////////////////////

DECLARE_TIMER(cAIMoveEnactor_NonMotEnactMoveGoal, Average);

HRESULT cAIMoveEnactor::NonMotEnactMoveGoal(const sAIMoveGoal & inputGoal, ulong deltaTime)
{
   AUTO_TIMER(cAIMoveEnactor_NonMotEnactMoveGoal);

   AutoAppIPtr(AINetServices);
   sAIMoveGoal goal = inputGoal;

   for (int i = 0; i < m_MoveRegulators.Size(); i++)
      m_MoveRegulators[i]->AssistGoal(inputGoal, &goal);

   // Calculate the control impulse from the move goal....
   sAIImpulse impulse;

   CalculateImpulse(goal, deltaTime, &impulse);

#if 0
   if (m_pAIState->GetMode() > kAIM_Efficient)
   {
      ConfigSpew("AI_NonMotSpew",("%d cur pos: %g, %g, %g\n%d move goal %g, %g, %g\n",GetID(),loc.x,loc.y,loc.z,GetID(),goal.dest.x,goal.dest.y,desiredZ));
      ConfigSpew("AI_NonMotSpew",("%d SetObjImpulse: %g, %g, %g, fac %g\n",GetID(),x,y,z,newFacing));
   }
#endif

   pAINetServices->SetObjImpulse(GetID(), impulse.vec.x, impulse.vec.y, impulse.vec.z, impulse.facing, (impulse.flags & kAIIF_Rotating));
   CalculateTargetVel(goal, deltaTime, &impulse);
   memcpy(m_pTargetVel, &impulse, sizeof(impulse));

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
