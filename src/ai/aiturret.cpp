/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiturret.cpp,v 1.13 2000/02/11 18:28:03 bfarquha Exp $
//
// AI Combat - turrets
//

// #define PROFILE_ON 1

#include <aiturret.h>

#include <comtools.h>
#include <appagg.h>
#include <cfgdbg.h>
#include <matrixs.h>

#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <traitman.h>
#include <objpos.h>
#include <iobjsys.h>
#include <rendprop.h>

#include <esnd.h>
#include <schema.h>

#include <phprop.h>
#include <phconst.h>
#include <physcast.h>
#include <physapi.h>

#include <playrobj.h>

#include <objhp.h>

#include <aiactmov.h>
#include <aiactrng.h>
#include <aiacttyp.h>
#include <aiaware.h>
#include <aigoal.h>
#include <aiprcore.h>
#include <airngwpn.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITurret
//

cAITurret::cAITurret()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAITurret::GetName()
{
   return "Turret";
}

///////////////////////////////////////
//
// How good a target is the object we are aware of?
// Pretty much just a copy of same func from cAICombat but add a pitch check
// Also use turret params to determine ranges

eAIPriority cAITurret::TargetPriority(ObjID targetID)
{
   eAIPriority priority = kAIP_None;

   if (CheckPitch(targetID))
   {
      priority = CheckRange(targetID);
      // Player bonus
      if (IsAPlayer(targetID))
         priority = (eAIPriority)((int)priority + 1);
   }
   else
      priority = kAIP_VeryLow;
   return priority;
}

///////////////////////////////////////

DECLARE_TIMER(cAITurret_OnActionProgress, Average);

STDMETHODIMP_(void) cAITurret::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAITurret_OnActionProgress);

   cAIDevice::OnActionProgress(pAction);

   if (pAction->GetResult() == kAIR_Success)
   {
      switch (pAction->GetType())
      {
      case kAIAT_FireGun:
         {
            SignalAction();
            int firePause = AIGetTurretParams(m_pAIState->GetID())->m_firePause;
            if (firePause>0)
               m_fireTimer.Set(eAITimerPeriod(firePause));
            ConfigSpew("TurretSpew", ("[%d] Finished fire\n", m_pAIState->GetID()));
         }
         break;
      }
   }
}

////////////////////////////////////////

BOOL cAITurret::CheckPitch(ObjID targetID)
{
   // Check our pitch
   float pitchEpsilon = AIGetTurretParams(m_pAIState->GetID())->m_pitchEpsilon;
   float pitch = m_pAIState->PitchTo(ObjPosGet(targetID)->loc.vec)*180/PI;
   return ((pitch<=pitchEpsilon) && (pitch>=-pitchEpsilon));
}

////////////////////////////////////////

BOOL cAITurret::CheckLOF(ObjID targetID, int* pSubModel)
{
   if (targetID == PlayerObject())
   {
      *pSubModel = PLAYER_HEAD;
      if (CheckLOF(targetID, *pSubModel))
         return TRUE;
      else
      {
         *pSubModel = PLAYER_BODY;
         return CheckLOF(targetID, *pSubModel);
      }
   }
   else
   {
      *pSubModel = 0;
      return CheckLOF(targetID, *pSubModel);
   }
}

///////////////////////////////////////////////////////
//
// Physcast callback
//

// passed to installed callbacks as data
struct sAITurretPhyscastData
{
   ObjID sourceID;   // ignore both of these
   ObjID targetID;
};

static BOOL AITurretPhyscastCallback(ObjID objID, const cPhysModel* pModel, sAITurretPhyscastData* pData)
{
   // ignore myself & my target
   if ((objID == (pData)->sourceID) || (objID == (pData)->targetID))
      return FALSE;

   // ignore destroyable objects
   int hp;
   if (ObjGetHitPoints(objID, &hp))
      return FALSE;

   return TRUE;
}

////////////////////////////////////////

BOOL cAITurret::CheckLOF(ObjID targetID, int subModel)
{
   Location hitLoc;
   Location targetLoc;
   mxs_vector targetVec;
   ObjID hitObjID;
   sAITurretPhyscastData data;

   if (targetID == OBJ_NULL)
      return FALSE;

   // Get submodel location
   PhysGetSubModLocation(targetID, subModel, &targetVec);
   MakeHintedLocationFromVector(&targetLoc, &targetVec, &ObjPosGet(targetID)->loc);

   // Setup callback data
   data.sourceID = GetID();
   data.targetID = targetID;
   PhysRaycastSetCallback(tPhyscastObjTest(AITurretPhyscastCallback), (void*)&data);

   // Actual physcast
   BOOL result = (PhysRaycast(Location(*(m_pAIState->GetPortalLocation())), targetLoc, &hitLoc, &hitObjID, 0.0,
      kCollideSphere|kCollideSphereHat|kCollideOBB|kCollideTerrain) == kCollideNone);
   PhysRaycastClearCallback();
   return result;
}

////////////////////////////////////////
//
// Check range.
// Returs VeryLow for out of range, Normal for above 1/2 max, VeryHigh for below 1/2 max
//

eAIPriority cAITurret::CheckRange(ObjID targetID)
{
   float maxRangeSq;
   float optRangeSq;
   float distSq = m_pAIState->DistSq(*GetObjLocation(targetID));
   eAIPriority priority;

   maxRangeSq = AIGetTurretParams(m_pAIState->GetID())->m_maxRange;
   optRangeSq = maxRangeSq/2;
   maxRangeSq = maxRangeSq*maxRangeSq;
   optRangeSq = optRangeSq*optRangeSq;

   if (distSq > maxRangeSq)
      priority = kAIP_VeryLow;
   else if (distSq < optRangeSq)
      priority = kAIP_VeryHigh;
   else
      priority = kAIP_Normal;
   return priority;
}

////////////////////////////////////////

STDMETHODIMP cAITurret::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   ObjID target;
   mxs_vector targetLoc;
   float* jointPos;
   floatang jointAng;
   floatang targetAng;
   floatang deltaAng;
   floatang facingAng;
   sAIDeviceParams* pDeviceParams;
   sAITurretParams* pTurretParams;
   cAIAction* pGunAction;
   BOOL continuing = FALSE;
   int subModel;

   cAIDevice::SuggestActions(pGoal, previous, pNew);

   // check for continuing existing gun action
   for (int i=0; i<previous.Size(); i++)
   {
      // This is a bit of a hack, we don't know what the type of our actions is, so we just ignore
      // the actions started by the device ability (joint rotate & scan). This will break if the
      // device ability ever starts other types of actions
      if (IsOwn(previous[i]) && previous[i]->InProgress() &&
         (previous[i]->GetType() != kAIAT_JointRotate) && (previous[i]->GetType() != kAIAT_JointScan)
         && (previous[i]->GetType() != kAIAT_JointSlide))
      {
         pNew->Append(previous[i]);
         previous[i]->AddRef();
         continuing = TRUE;
      }
   }
   if (continuing)
      return S_OK;

   if (GetDeviceState() != kAIDeviceActive)
      return S_OK;

   // Setup
   pDeviceParams = AIGetDeviceParams(m_pAIState->GetID());
   pTurretParams = AIGetTurretParams(m_pAIState->GetID());

   target = GetTarget();
   // we *should* always have a target by now
   if (target == OBJ_NULL)
      return E_FAIL;
   targetLoc = ObjPosGet(target)->loc.vec;
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[pDeviceParams->m_jointRotate]);
   facingAng = m_pAIState->GetFacingAng()+jointAng;
   targetAng = m_pAIState->AngleTo(targetLoc);
   deltaAng = fabs(Delta(facingAng, targetAng).value);

   // if not fully alert, then don't fire
   if (m_pAIState->GetAlertness()<kAIAL_High)
      return S_OK;

   // Fire if we aren't already doing so & we can see the target
   const sAIAwareness * pAwareness = m_pAI->GetAwareness(target);

   if (m_fireTimer.Expired() &&
      (pAwareness && (pAwareness->flags & kAIAF_CanRaycast)) &&
      CheckLOF(target, &subModel) &&
      (deltaAng<pTurretParams->m_fireEpsilon) &&
      (CheckRange(target)>kAIP_VeryLow) &&
      CheckPitch(target))
   {
      pGunAction = m_gunActionFactoryFn(this, target, subModel);
      pNew->Append(pGunAction);
      ConfigSpew("TurretSpew", ("[%d] Fire\n", m_pAIState->GetID()));
   }
   return S_OK;
}

////////////////////////////////////////

ObjID cAITurret::GetRangedWeapon(void)
{
   AutoAppIPtr_(TraitManager, pTraitMan);
   ObjID gun = OBJ_NULL;

   // Get gun from link from our archetype
   ILinkQuery *query = g_pAIRangedWeaponLinks->Query(pTraitMan->GetArchetype(GetID()), LINKOBJ_WILDCARD);
   if (!query->Done())
   {
      sLink link;
      query->Link(&link);

      gun = link.dest;
   }
   else
      Warning(("cAITurret::GetRangedWeapon: no Ranged Weapon link for AI %d\n", GetID()));

   SafeRelease(query);
   return gun;
}











