/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasctm.h,v 1.31 2000/02/17 23:34:31 adurant Exp $
//
// Template file for non-inline members of the AI component base class
//
#pragma once

#ifndef __AIBASCTM_H
#define __AIBASCTM_H

#include <appagg.h> // for AI manager

#include <aiapiiai.h>
#include <aitagtyp.h>

#define AIUPDATE 0

#ifdef AIUPDATE
#include <config.h>
#endif 

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cAIComponentBase
//

#define AI_COMPONENT_BASE_TEMPLATE  template <class INTERFACE, const GUID * pIID_INTERFACE>
#define AI_COMPONENT_BASE           cAIComponentBase<INTERFACE, pIID_INTERFACE>
#define AI_COMPONENT_BASE_BASE      cCTUnaggregated<INTERFACE, pIID_INTERFACE, kCTU_Default>

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP AI_COMPONENT_BASE::QueryInterface(REFIID id, void ** ppI)
{
   if (IsEqualGUID(id, IID_IAIComponent))
   {
      AddRef();
      *ppI = (IAIComponent *)this;
      return S_OK;
   }
   return AI_COMPONENT_BASE_BASE::QueryInterface(id, ppI);
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::GetSubComponents(cAIComponentPtrs *)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::ConnectComponent(IInternalAI * pOuterAI, unsigned ** ppNotifications)
{
   m_pAI            = pOuterAI;
   m_pAIState       = m_pAI->GetState();
   *ppNotifications = &m_notifications;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::Init()
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::End()
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(IInternalAI *) AI_COMPONENT_BASE::AccessOuterAI()
{
   return m_pAI;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnSimStart()
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnSimEnd()
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnBeginFrame()
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnEndFrame()
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnUpdateProperties(eAIPropUpdateKind kind)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnActionProgress(IAIAction *)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnGoalProgress(const cAIGoal *)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnModeChange(eAIMode, eAIMode)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnGoalChange(const cAIGoal *, const cAIGoal *)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnActionChange(IAIAction *, IAIAction *)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnAlertness(ObjID source,
                                                   eAIAwareLevel previous,
                                                   eAIAwareLevel current,
                                                   const sAIAlertness * pRaw)
{
}


AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnAwareness(ObjID source,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw)
{
}



///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnHighAlert(ObjID source,
                                                   eAIAwareLevel previous,
                                                   eAIAwareLevel current,
                                                   const sAIAlertness * pRaw)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnDamage(const sDamageMsg * pMsg, ObjID realCulpritID)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnDeath(const sDamageMsg * pMsg)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnWeapon(eWeaponEvent, ObjID, ObjID)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnSound(const sSoundInfo *, const sAISoundType *)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnFastObj(ObjID firer, ObjID projectile, const mxs_vector & velocity)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnFoundBody(ObjID body)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnFoundSuspicious(ObjID suspobj)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnScriptCommand(eAIScriptCmd command, void *)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnProperty(IProperty *, const sPropertyListenMsg *)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnDefend(const sAIDefendPoint *)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnSignal(const sAISignal *)
{

}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnWitnessCombat()
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnCollision(const sPhysListenMsg*)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnTripwire(ObjID objID, BOOL enter)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnFrustration(ObjID source, ObjID dest, IAIPath *pPath)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnGoalsReset(const cAIGoal *)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(void) AI_COMPONENT_BASE::OnGameEvent(void *)
{
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(BOOL) AI_COMPONENT_BASE::PreSave()
{
   return TRUE;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(BOOL) AI_COMPONENT_BASE::Save(ITagFile *)
{
   return TRUE;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(BOOL) AI_COMPONENT_BASE::PostSave()
{
   return TRUE;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(BOOL) AI_COMPONENT_BASE::PreLoad()
{
   return TRUE;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(BOOL) AI_COMPONENT_BASE::Load(ITagFile *)
{
   return TRUE;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
STDMETHODIMP_(BOOL) AI_COMPONENT_BASE::PostLoad()
{
   return TRUE;
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
void AI_COMPONENT_BASE::SetNotifications(unsigned fNotifications)
{
   m_notifications |= fNotifications;
   if (m_notifications&kAICN_Collision)
   {
      AutoAppIPtr(AIManager);
      pAIManager->ListenForCollisions(m_pAI->GetState()->GetID());
   }
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
void AI_COMPONENT_BASE::ClearNotifications(unsigned fNotifications)
{
   m_notifications &= ~fNotifications;
   if (m_notifications&kAICN_Collision)
   {
      AutoAppIPtr(AIManager);
      pAIManager->UnlistenForCollisions(m_pAI->GetState()->GetID());
   }
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
void AI_COMPONENT_BASE::SaveNotifications(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &m_notifications);
}

///////////////////////////////////////

AI_COMPONENT_BASE_TEMPLATE
void AI_COMPONENT_BASE::LoadNotifications(ITagFile * pTagFile)
{
#ifdef AIUPDATE // sometimes, this it is necessary to reset notification, if some were added or removed.
   if (!config_is_defined("AIUpdate"))
#endif
      AITagMove(pTagFile, &m_notifications);
#ifdef AIUPDATE
   else
   {
      unsigned nTempNotifications;
      AITagMove(pTagFile, &nTempNotifications);
      if (nTempNotifications != m_notifications)
         Warning(("Updating %d's notifications from %x to %x\n", GetID(), nTempNotifications, m_notifications));
   }
#endif
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AICMPTEM_H */
