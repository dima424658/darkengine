/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiswa.cpp,v 1.3 1999/02/09 09:41:29 JON Exp $
//

#include <lg.h>
#include <mprintf.h>

#include <aiactloc.h>
#include <aiapiiai.h>
#include <aiapipth.h>
#include <aidebug.h>
#include <aipathdb.h>

#include <shkaiswa.h>
#include <shkaipr.h>

// Must be last header
#include <dbmem.h>

#ifndef SHIP
#define AI_COMBAT_DEBUGGING
#endif

#ifdef AI_COMBAT_DEBUGGING
#define _SWD                             "AISWAWatch %d: "
#define _SWD_P                           GetID()
#define _SWAWatchObj()                   (AIIsWatched(Cbt, GetID()))
#define _SWAWatchPrint(str)              do { if (_SWAWatchObj()) mprintf(_SWD##str,_SWD_P); } while (0)
#define _SWAWatchPrint1(str,v1)          do { if (_SWAWatchObj()) mprintf(_SWD##str,_SWD_P,v1); } while (0)
#define _SWAWatchPrint2(str,v1,v2)       do { if (_SWAWatchObj()) mprintf(_SWD##str,_SWD_P,v1,v2); } while (0)
#define _SWAWatchPrint3(str,v1,v2,v3)    do { if (_SWAWatchObj()) mprintf(_SWD##str,_SWD_P,v1,v2,v3); } while (0)
#define _SWAWatchPrint4(str,v1,v2,v3,v4) do { if (_SWAWatchObj()) mprintf(_SWD##str,_SWD_P,v1,v2,v3,v4); } while (0)
#define _SWAModeName(mode)               g_AISwarmModeNames[mode]
const char* g_AISwarmModeNames[] =
 { "Close", "BackOff" };
#else
#define _SWAWatchPrint(str)
#define _SWAWatchPrint1(str,v1)
#define _SWAWatchPrint2(str,v1,v2)
#define _SWAWatchPrint3(str,v1,v2,v3)
#define _SWAWatchPrint4(str,v1,v2,v3,v4)
#define _SWAModeName(mode) 
#endif

//////////////////////////////////////////////////////////////////////////////

#define YA_PI (3.14159265358979323846) // yet another pi

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISwarm
//

cAISwarm::cAISwarm():
   m_giveupTimer(3000, 5000)   
{
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISwarm::Init()
{
   cAICombat::Init();
   SetMode(kAISwM_Close);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAISwarm::GetName()
{
   return "Swarm Ability";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAISwarm::Save(ITagFile * pTagFile)
{
   // @TODO: 
   return TRUE;   
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAISwarm::Load(ITagFile * pTagFile)
{
   // @TODO:
  return TRUE;   
}

STDMETHODIMP_(void) cAISwarm::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);
   if (IsOwn(pAction) && m_giveupTimer.Expired())
      SignalAction();
}

void cAISwarm::SetMode(eAISwarmMode mode)
{
   switch(mode)
   {
   case kAISwM_Close:
      break;
   case kAISwM_BackOff:
      {
         m_gotoLoc = GetTargetLoc();
         cMxsVector offset(ShockAIGetBackOffDist(m_pAIState->GetID()), 0, 0);
         cMxsVector rotatedOffset;
         // rotate randomly
         mx_rot_z_vec(&rotatedOffset, &offset, AIRandom(0, 0xffff));
         mx_addeq_vec(&m_gotoLoc, &rotatedOffset);
      }
      break;
   default:
      Warning(("cAISwarm::SetMode - unrecognized state %d\n", mode));
      break;
   }
   m_mode = mode;
   _SWAWatchPrint1("Mode is %s\n", _SWAModeName(mode));
}

//////////////////////////////////////////////

cAILocoAction* cAISwarm::CreateBackoffAction(float distance)
{
   mxs_vector target;
   const mxs_vector& aiLoc = *m_pAIState->GetLocation();
   IAIPathfinder* pPathfinder = m_pAI->AccessPathfinder();
   tAIPathCellID destCell = 0;
   BOOL found = FALSE;
   floatang angle;
   
   if (!pPathfinder)
   {
      Warning(("cAISwarmAbility::CreateBackoffAction - can't get pathfinder\n"));
      return NULL;
   }
   
   // try to find backoff point
   for (int i = 0; (i < 8) && !found; i++)
   {
      angle.value = DEGREES(AIRandom(0, 360));
      ProjectFromLocationOnZPlane(aiLoc, distance, angle, &target);
      if (pPathfinder->Pathcast(target, &destCell))
      {
         Assert_(destCell);
         target.z = g_AIPathDB.GetZAtXY(destCell, target) + 3.0;
         found = TRUE;
      }
   }
   
   if (!found)
      return NULL;

   // make the loco action
   cAILocoAction * pAction = CreateLocoAction();
   pAction->Set(target, kAIS_Fast);
      
   return pAction;
}

////////////////////////////////////////////////

STDMETHODIMP cAISwarm::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   float targDist = sqrt(m_pAIState->DistSq(GetTargetLoc()));
   if ((m_mode == kAISwM_Close) && (targDist<ShockAIGetCloseDist(m_pAIState->GetID())))
      SetMode(kAISwM_BackOff);
   else if ((m_mode == kAISwM_BackOff) && (targDist>ShockAIGetCloseDist(m_pAIState->GetID())))
      SetMode(kAISwM_Close);
   cAILocoAction * pLocoAction;
   switch(m_mode)
   {
   case kAISwM_Close:
      pLocoAction = CreateLocoAction();
      pLocoAction->Set(GetTargetLoc(), GetTarget(), kAIS_Fast);
      break;
   case kAISwM_BackOff:
      pLocoAction = CreateBackoffAction(ShockAIGetBackOffDist(m_pAIState->GetID()));
      break;
   default:
      Warning(("cAISwarm::SuggestActions - unrecognized mode %d\n", m_mode));
      break;
   }
   if (pLocoAction != NULL)
   {
      pNew->Append(pLocoAction);
      m_giveupTimer.Reset();
   }
   return S_OK;
}
