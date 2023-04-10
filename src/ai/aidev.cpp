/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidev.cpp,v 1.20 2000/03/22 15:41:35 adurant Exp $
//
// AI Device Ability
//

// #define PROFILE_ON 1

#include <aidev.h>

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

#include <aiactjr.h>
#include <aiactjs.h>
#include <aiactmov.h>
#include <aiactrng.h>
#include <aiacttyp.h>
#include <aiaware.h>
#include <aigoal.h>
#include <airngwpn.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDevice
//

cAIDevice::cAIDevice():
   m_state(kAIDeviceInactive)
{
}

///////////////////////////////////////

cAIDevice::~cAIDevice()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIDevice::GetName()
{
   return "Device";
}

///////////////////////////////////////

void cAIDevice::SetState(eAIDeviceState state)
{
   switch (state)
   {
   case kAIDeviceActivating:
      {
         sAIDeviceParams *pParams = AIGetDeviceParams(m_pAIState->GetID());
         if (!IsValidJoint(pParams->m_jointActivate))
            SetState(kAIDeviceActive);
         else
            m_state = state;
         ConfigSpew("DeviceSpew", ("[%d] Activating\n", m_pAIState->GetID()));
      }
      break;
   case kAIDeviceDeactivating:
      {
         sAIDeviceParams *pParams = AIGetDeviceParams(m_pAIState->GetID());
         if (!IsValidJoint(pParams->m_jointActivate))
            SetState(kAIDeviceInactive);
         else
            m_state = state;
         ConfigSpew("DeviceSpew", ("[%d] Deactivating\n", m_pAIState->GetID()));
      }
      break;
   case kAIDeviceActive:
      m_state = state;
      ConfigSpew("DeviceSpew", ("[%d] Activated\n", m_pAIState->GetID()));
      break;
   case kAIDeviceInactive:
      m_state = state;
      ConfigSpew("DeviceSpew", ("[%d] Deactivated\n", m_pAIState->GetID()));
      break;
   default:
      Warning(("cAIDevice::SetState: bad state %d for AI %d\n", state, m_pAIState->GetID()));
      break;
   }
}

///////////////////////////////////////

BOOL cAIDevice::ReadyToDeactivate(void)
{
   // @TODO: propertize
   return ((m_pAIState->GetAlertness()<=kAIAL_Low) && (m_state != kAIDeviceInactive));
}

///////////////////////////////////////

BOOL cAIDevice::ReadyToActivate(void)
{
   // @TODO: propertize
   return ((m_pAIState->GetAlertness()>=kAIAL_High) && (m_state != kAIDeviceActive));
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDevice::OnAlertness(ObjID source, eAIAwareLevel previous,  eAIAwareLevel current, const sAIAlertness * pRaw)
{
   cAICombat::OnAlertness(source, previous,  current, pRaw);

   if (ReadyToDeactivate())
     SignalGoal();
   if (ReadyToActivate())
     SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP cAIDevice::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   cAICombat::SuggestGoal(pPrevious, ppNew);
   if ((*ppNew == NULL) && ReadyToDeactivate())
   {
      // idle goal doesn't really make sense, but it doesn't matter either
      cAIIdleGoal * pGoal = new cAIIdleGoal(this);
      pGoal->priority = kAIP_Normal;
      *ppNew = pGoal;
   }
   if ((*ppNew == NULL) && ReadyToActivate())
     {
       //gonna overload idle goal here
       cAIIdleGoal * pGoal = new cAIIdleGoal(this);
       pGoal->priority = kAIP_Normal;
       *ppNew = pGoal;
     }
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDevice::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAICombat::OnGoalChange(pPrevious, pGoal);

   // If we got a new goal...
   if (IsOwn(pGoal))
   {
      if ((pGoal->GetType() == kAIGT_Attack) && (m_state != kAIDeviceActive) && (m_state != kAIDeviceActivating))
         SetState(kAIDeviceActivating);
      else if ((pGoal->GetType() == kAIGT_Idle))
      {
         if (ReadyToActivate() && (m_state != kAIDeviceActivating))
            SetState(kAIDeviceActivating);
         else if (ReadyToDeactivate() && (m_state != kAIDeviceDeactivating))
            SetState(kAIDeviceDeactivating);
      }
      SignalAction();
   }
}

///////////////////////////////////////

DECLARE_TIMER(cAIDevice_OnActionProgress, Average);

STDMETHODIMP_(void) cAIDevice::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIDevice_OnActionProgress);

   cAICombat::OnActionProgress(pAction);

   if (IsOwn(pAction) && pAction->GetResult() == kAIR_Success)
   {
      sAIDeviceParams *pParams = AIGetDeviceParams(m_pAIState->GetID());
      switch (pAction->GetType())
      {
         case kAIAT_JointSlide:
         {
            Assert_(IsValidJoint(pParams->m_jointActivate));
            if (ObjJointPos(m_pAIState->GetID())[pParams->m_jointActivate] == pParams->m_activePos)
               SetState(kAIDeviceActive);
            else if (ObjJointPos(m_pAIState->GetID())[pParams->m_jointActivate] == pParams->m_inactivePos)
               SetState(kAIDeviceInactive);
            ConfigSpew("DeviceSpew", ("[%d] Finished slide\n", m_pAIState->GetID()));
            break;
         }
         case kAIAT_JointRotate:
         {
            ConfigSpew("DeviceSpew", ("[%d] Finished rotate\n", m_pAIState->GetID()));
            //ok, if we were rotating, and that rotation joint is our
            //active joint, then we need to check things
            
            //if our activate is rotate
            //This has a potential hazard if I am in some "superactive"
            //state (which doesn't currently exist) of setting me back
            //to active when some OTHER joint rotate finishes...
            //I find this unlikely, and note that Slide above has the
            //same problem.  If there is a way to cast to a jointrotate
            //action then I could query it for joint ID, must ask someone
            //about this.  AMSD
            if ((pParams->m_activateRotate)) 
            {
               Assert_(IsValidJoint(pParams->m_jointActivate));
               if (((int)(ObjJointPos(m_pAIState->GetID())[pParams->m_jointActivate] - pParams->m_activePos + 0.5)==0) && (m_state == kAIDeviceActivating))
                  SetState(kAIDeviceActive);
               else if (((int)(ObjJointPos(m_pAIState->GetID())[pParams->m_jointActivate] - pParams->m_inactivePos + 0.5)==0)  && (m_state == kAIDeviceDeactivating))
                  SetState(kAIDeviceInactive);
            }
            break;
            }
         case kAIAT_FireGun:
         {
            ConfigSpew("DeviceSpew", ("[%d] Finished fire\n", m_pAIState->GetID()));
            break;
         }
      }
      SignalAction();
   }
   // check our facing
   else if (ShouldChangeFacing())
      SignalAction();
}

////////////////////////////////////////

BOOL cAIDevice::IsDoing(const cAIActions &actions, tAIActionType type)
{
   int i;

   for (i=0; i<actions.Size(); i++)
      if (actions[i]->GetType() == type)
         return TRUE;
   return FALSE;
}

////////////////////////////////////////
// Do we need to rotate?
//
BOOL cAIDevice::ShouldChangeFacing(void)
{
   ObjID target;

   if ((target = GetTarget()) == OBJ_NULL)
      return FALSE;
   return (FacingDelta(target)>AIGetDeviceParams(m_pAIState->GetID())->m_facingEpsilon);
}

////////////////////////////////////////
// Calc angular offset to target accounting for joint angle
//
floatang cAIDevice::FacingDelta(ObjID target)
{
   mxs_vector targetLoc;
   float *jointPos;
   floatang jointAng;
   floatang targetAng;

   // This better not happen too often...
   if (target == OBJ_NULL)
      return 0;

   targetLoc = ObjPosGet(target)->loc.vec;
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[AIGetDeviceParams(m_pAIState->GetID())->m_jointRotate]);
   targetAng = m_pAIState->AngleTo(targetLoc);
   return fabs(Delta(m_pAIState->GetFacingAng()+jointAng, targetAng).value);
}

////////////////////////////////////////

HRESULT cAIDevice::ActiveSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   ObjID target;
   mxs_real projRange = 10;
   int i;
   sAIDeviceParams *pParams;

   // Keep doing actions that aren't finished yet
   for (i=0; i<previous.Size(); i++)
      if ((previous[i]->GetType() == kAIAT_JointRotate) &&
         ((previous[i]->GetResult() == kAIR_NoResult) || (previous[i]->GetResult() == kAIR_NoResultSwitch)))
      {
         pNew->Append(previous[i]);
         previous[i]->AddRef();
      }

   if ((target = GetTarget()) == OBJ_NULL)
      return S_OK;

   // Get device parameters. Return if no rotation joint specified.
   pParams = AIGetDeviceParams(m_pAIState->GetID());
   if (!IsValidJoint(pParams->m_jointRotate))
      return S_OK;

   // Rotate if necessary
   if (!IsDoing(*pNew, kAIAT_JointRotate) && (FacingDelta(target)>pParams->m_facingEpsilon))
   {
      m_pRotateAction = new cAIJointRotateAction(this);
      m_pRotateAction->Set(pParams->m_jointRotate, target);
      pNew->Append(m_pRotateAction);
      ConfigSpew("DeviceSpew", ("[%d] Starting rotate action\n", m_pAIState->GetID()));
   }

   return S_OK;
}

////////////////////////////////////////

HRESULT cAIDevice::ActivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   int i;
   sAIDeviceParams *pParams;

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
   pParams = AIGetDeviceParams(m_pAIState->GetID());
   // should only be here if we have a joint to activate
   Assert_(IsValidJoint(pParams->m_jointActivate));
   if (!pParams->m_activateRotate)  //i.e., slide
     {
       cAIJointSlideAction *pSlideAction = new cAIJointSlideAction(this);
       pSlideAction->Set(pParams->m_jointActivate, pParams->m_activePos, pParams->m_activateSpeed);
       pNew->Append(pSlideAction);
       ConfigSpew("DeviceSpew", ("[%d] Starting raise action\n", m_pAIState->GetID()));
     }
   else  // ok, rotate instead
     {
       cAIJointRotateAction *pRotateAction = new cAIJointRotateAction(this);
       // @HACK: Joint rotate action uses Facing Ang + Joint position.  We don't want Facing Ang
       // for an activate, since we activate on a joint that should be facing independent.  
       // AMSD 3/21/00
       pRotateAction->Set(pParams->m_jointActivate,
                          (floatang)DEGREES(pParams->m_activePos + RADIANS(m_pAIState->GetFacingAng().value)),
                          DEGREES(0.5));
       pNew->Append(pRotateAction);
       ConfigSpew("DeviceSpew", ("[%d] Starting rotate activate action\n", m_pAIState->GetID()));
     }
   // play sound
   // @TODO: if activate & deactivate are the same tags, then this should be moved into the slide action
   cTagSet tagSet("Event Activate");
   ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL);
   return S_OK;
}

////////////////////////////////////////

HRESULT cAIDevice::DeactivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   int i;
   sAIDeviceParams *pParams;
   float *jointPos;
   floatang jointAng;

   // Keep doing a raise action that isn't finished yet
   for (i=0; i<previous.Size(); i++)
      if (((previous[i]->GetType() == kAIAT_JointSlide) || (previous[i]->GetType() == kAIAT_JointRotate)) &&
         ((previous[i]->GetResult() == kAIR_NoResult) || (previous[i]->GetResult() == kAIR_NoResultSwitch)))
      {
         pNew->Append(previous[i]);
         previous[i]->AddRef();
         return S_OK;
      }

   pParams = AIGetDeviceParams(m_pAIState->GetID());

   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[AIGetDeviceParams(m_pAIState->GetID())->m_jointRotate]);
   if (fabs(jointAng.value)>pParams->m_facingEpsilon)
   {
      // rotate joint back to zero
      m_pRotateAction = new cAIJointRotateAction(this);
      m_pRotateAction->Set(pParams->m_jointRotate, m_pAIState->GetFacingAng(), pParams->m_facingEpsilon);
      pNew->Append(m_pRotateAction);
      ConfigSpew("DeviceSpew", ("[%d] Starting rotate to zero action\n", m_pAIState->GetID()));
   }
   else
   {
      // otherwise, start slide-down or rotate down
      Assert_((pParams != NULL) && IsValidJoint(pParams->m_jointActivate));
      if (!pParams->m_activateRotate) //slide action
      {
         cAIJointSlideAction *pSlideAction = new cAIJointSlideAction(this);
         pSlideAction->Set(pParams->m_jointActivate, pParams->m_inactivePos, pParams->m_activateSpeed);
         pNew->Append(pSlideAction);
         ConfigSpew("DeviceSpew", ("[%d] Starting lower action\n", m_pAIState->GetID()));
      }
      else //rotate action
      {
         cAIJointRotateAction *pRotateAction = new cAIJointRotateAction(this);
         // @HACK See comment in ActivateSuggestActions AMSD 3/21/00
         pRotateAction->Set(pParams->m_jointActivate,
                            (floatang)DEGREES(pParams->m_inactivePos + RADIANS(m_pAIState->GetFacingAng().value)),
                            DEGREES(0.5));
         pNew->Append(pRotateAction);
         ConfigSpew("DeviceSpew", ("[%d] Starting rotate deactivate action\n", m_pAIState->GetID()));
      }

      // play sound
      // @TODO: if activate & deactivate are the same tags, then this should be moved into the slide action
      cTagSet tagSet("Event Deactivate");
      ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL);
   }
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cAIDevice::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   switch (m_state)
   {
   case kAIDeviceActivating:
      return ActivateSuggestActions(pGoal, previous, pNew);
   case kAIDeviceDeactivating:
      return DeactivateSuggestActions(pGoal, previous, pNew);
   case kAIDeviceInactive:
      return S_OK;
   case kAIDeviceActive:
      return ActiveSuggestActions(pGoal, previous, pNew);
   default:
      Warning(("cAIDevice::SuggestActions - unknown state %d\n", m_state));
      return S_OK;
   }

}

////////////////////////////////////////

