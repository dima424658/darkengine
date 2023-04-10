/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaitrt.cpp,v 1.8 1998/07/28 21:21:15 TOML Exp $
//
// AI Combat - turrets 
//

#include <shkaitrt.h>

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
#include <physapi.h>

#include <aiactmov.h>
#include <aiactrng.h>
#include <aiacttyp.h>
#include <aiaware.h>
#include <aigoal.h>
#include <airngwpn.h>
#include <aisnsdat.h>

#include <shkgunpr.h>
#include <shkaijra.h>
#include <shkaijsa.h>
#include <shkaigna.h>
#include <shkgnapi.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombatTurret
//

cAICombatTurret::cAICombatTurret():
   m_state(kAITurretInactive),
   m_rotateSchemaHandle(-1)
{
}

///////////////////////////////////////

cAICombatTurret::~cAICombatTurret()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAICombatTurret::GetName()
{
   return "Turret combat";
}

///////////////////////////////////////

STDMETHODIMP cAICombatTurret::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   cAICombat::SuggestGoal(pPrevious, ppNew);

   if (*ppNew == NULL)
   {
      cAIIdleGoal * pGoal = new cAIIdleGoal(this);
      pGoal->priority = kAIP_Low;
      *ppNew = pGoal;
   }
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombatTurret::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAICombat::OnGoalChange(pPrevious, pGoal);

   if (IsOwn(pGoal) && (pGoal->GetType() == kAIGT_Attack) && (m_state != kAITurretActive) && (m_state != kAITurretActivating))
   {
      m_state = kAITurretActivating;
      SignalAction();
   }
   else if ((!IsOwn(pGoal) || (pGoal->GetType() != kAIGT_Attack)) && ((m_state == kAITurretActive) || (m_state == kAITurretActivating)))
   {
      m_state = kAITurretDeactivating;
      SignalAction();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombatTurret::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);

   if (pAction->GetResult() == kAIR_Success)
   {
      sAITurretParams *pParams;
      g_pAITurretProperty->Get(m_pAIState->GetID(), &pParams);
      switch (pAction->GetType())
      {
         case kAIAT_JointSlide: 
         {
            if (ObjJointPos(m_pAIState->GetID())[pParams->m_jointRaise] == pParams->m_activePos)
            {
               m_state = kAITurretActive;
               ConfigSpew("TurretSpew", ("[%d] Activated\n", m_pAIState->GetID()));
            }
            else if (ObjJointPos(m_pAIState->GetID())[pParams->m_jointRaise] == pParams->m_inactivePos)
            {
               m_state = kAITurretInactive;
               ConfigSpew("TurretSpew", ("[%d] Deactivated\n", m_pAIState->GetID()));
            }
            SignalAction();
            ConfigSpew("TurretSpew", ("[%d] Finished slide\n", m_pAIState->GetID()));
            break;
         }
         case kAIAT_JointRotate:
         {
            SignalAction();
            ConfigSpew("TurretSpew", ("[%d] Finished rotate\n", m_pAIState->GetID()));
            if (m_rotateSchemaHandle != -1)
            {
               // @TODO: we should really have a callback to make sure that the schema didn't die 
               // underneath us
               SchemaPlayHalt(m_rotateSchemaHandle);
               m_rotateSchemaHandle = -1;
            }
            cTagSet tagSet("Event Rotate");
            tagSet.Add(cTag("LoopState", "stop"));
            m_rotateSchemaHandle = ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL); 
            break;
         }
         case kAIAT_FireGun:    
         {
            SignalAction();
            ConfigSpew("TurretSpew", ("[%d] Finished fire\n", m_pAIState->GetID()));
            break;
         }
      }
   }
}  

////////////////////////////////////////

BOOL cAICombatTurret::IsDoing(const cAIActions &actions, tAIActionType type)
{
   int i;

   for (i=0; i<actions.Size(); i++)
      if (actions[i]->GetType() == type)
         return TRUE;
   return FALSE;
}

////////////////////////////////////////

HRESULT cAICombatTurret::ActiveSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   ObjID target;
   mxs_real projRange = 10;
   mxs_vector targetLoc;
   float *jointPos;
   floatang jointAng;
   floatang targetAng;
   floatang deltaAng;
   int i;
   sAITurretParams *pParams;

   // Keep doing actions that aren't finished yet
   for (i=0; i<previous.Size(); i++)
      if ((previous[i]->GetResult() == kAIR_NoResult) || (previous[i]->GetResult() == kAIR_NoResultSwitch))
      {
         pNew->Append(previous[i]);
         previous[i]->AddRef();
      }

   // Setup
   g_pAITurretProperty->Get(m_pAIState->GetID(), &pParams);
   target = GetTarget();
   // we *should* always have a target by now
   if (target == OBJ_NULL)
      return E_FAIL;
   targetLoc = ObjPosGet(target)->loc.vec;
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[pParams->m_jointRotate]);
   targetAng = m_pAIState->AngleTo(targetLoc);
   deltaAng = fabs(Delta(jointAng, targetAng).value);

   // Rotate if necessary
   if (!IsDoing(*pNew, kAIAT_JointRotate) && (deltaAng>pParams->m_facingEpsilon))
   {
      m_pRotateAction = new cAIJointRotateAction(this);
      m_pRotateAction->Set(pParams->m_jointRotate, target, 0.001);
      pNew->Append(m_pRotateAction);
      ConfigSpew("TurretSpew", ("[%d] Starting rotate action\n", m_pAIState->GetID()));
      // play sound
      cTagSet tagSet("Event Rotate");
      tagSet.Add(cTag("LoopState", "loop"));
      m_rotateSchemaHandle = ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL); 
   }

   // if not fully alert, then don't fire
   if (m_pAIState->GetAlertness()<kAIAL_High)
      return S_OK;

   // Fire if we aren't already doing so & we can see the target
   const sAIAwareness * pAwareness = m_pAI->GetAwareness(target);
   if (!IsDoing(*pNew, kAIAT_FireGun) && 
      (pAwareness && (pAwareness->flags & kAIAF_CanRaycast)) && 
      (deltaAng<pParams->m_fireEpsilon))
   {
      m_pGunAction = new cAIGunAction(this);
      m_pGunAction->Set(target, pParams->m_jointRotate);
      pNew->Append(m_pGunAction);
      ConfigSpew("TurretSpew", ("[%d] Fire\n", m_pAIState->GetID()));
   }

   return S_OK;
}

////////////////////////////////////////

HRESULT cAICombatTurret::ActivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   int i;
   sAITurretParams *pParams;

   // Keep doing a raise action that isn't finished yet
   for (i=0; i<previous.Size(); i++)
      if ((previous[i]->GetType() == kAIAT_JointSlide) &&
         ((previous[i]->GetResult() == kAIR_NoResult) || (previous[i]->GetResult() == kAIR_NoResultSwitch)))
      {
         pNew->Append(previous[i]);
         previous[i]->AddRef();
         return S_OK;
      }
      
   // otherwise, start slide-up
   cAIJointSlideAction *pSlideAction = new cAIJointSlideAction(this);
   g_pAITurretProperty->Get(m_pAIState->GetID(), &pParams);
   pSlideAction->Set(pParams->m_jointRaise, pParams->m_activePos, pParams->m_raiseSpeed);
   pNew->Append(pSlideAction);
   ConfigSpew("TurretSpew", ("[%d] Starting raise action\n", m_pAIState->GetID()));
   // play sound
   cTagSet tagSet("Event Activate");
   ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL); 
   return S_OK;
}

////////////////////////////////////////

HRESULT cAICombatTurret::DeactivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   int i;
   sAITurretParams *pParams;

   // Keep doing a raise action that isn't finished yet
   for (i=0; i<previous.Size(); i++)
      if ((previous[i]->GetType() == kAIAT_JointSlide) &&
         ((previous[i]->GetResult() == kAIR_NoResult) || (previous[i]->GetResult() == kAIR_NoResultSwitch)))
      {
         pNew->Append(previous[i]);
         previous[i]->AddRef();
         return S_OK;
      }
      
   // otherwise, start slide-down
   cAIJointSlideAction *pSlideAction = new cAIJointSlideAction(this);
   g_pAITurretProperty->Get(m_pAIState->GetID(), &pParams);
   pSlideAction->Set(pParams->m_jointRaise, pParams->m_inactivePos, pParams->m_raiseSpeed);
   pNew->Append(pSlideAction);
   ConfigSpew("TurretSpew", ("[%d] Starting lower action\n", m_pAIState->GetID()));
   // play sound
   cTagSet tagSet("Event Activate");
   ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL); 
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cAICombatTurret::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   switch (m_state)
   {
   case kAITurretActivating:
      return ActivateSuggestActions(pGoal, previous, pNew);
   case kAITurretDeactivating:
      return DeactivateSuggestActions(pGoal, previous, pNew);
   case kAITurretInactive:
      return S_OK;
   case kAITurretActive:
      return ActiveSuggestActions(pGoal, previous, pNew);
   default:
      Warning(("cAICombatTurret::SuggestActions - unknown state %d\n", m_state));
      return S_OK;
   }

}

////////////////////////////////////////

STDMETHODIMP_(void) cAICombatTurret::OnAlertness(ObjID source, eAIAwareLevel previous, 
                                                 eAIAwareLevel current,
                                                 const sAIAlertness * pRaw)
{
   /*
   cAICombat::OnAlertness(source, previous, current, pRaw);
   */
}

////////////////////////////////////////

ObjID cAICombatTurret::GetGun(void)
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
      Warning(("cAICombatTurret::GetGun: no Ranged Weapon link for AI %d\n", GetID()));

   SafeRelease(query);
   return gun;
}











