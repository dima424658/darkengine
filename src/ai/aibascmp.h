/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibascmp.h,v 1.31 2000/02/16 12:06:13 bfarquha Exp $
//
// Base implementation of an AI component as a template
//
#pragma once

#ifndef __AIBASCMP_H
#define __AIBASCMP_H

#include <aiapiiai.h>
#include <aiapicmp.h>

class cAIState;

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cAIComponentBase
//
// Classes that only implement IAIComponent may derive from "cAIComponent"
//

#undef INTERFACE

template <class INTERFACE, const GUID * pIID_INTERFACE>
class cAIComponentBase : public cCTUnaggregated<INTERFACE, pIID_INTERFACE, kCTU_Default>
{
public:
   cAIComponentBase()
    : m_pAI(NULL),
      m_notifications(0)
   {
   }

   STDMETHOD (QueryInterface)(REFIID id, void ** ppI);

   // Hook up the component
   STDMETHOD_(void, GetSubComponents)(cAIComponentPtrs *);
   STDMETHOD_(void, ConnectComponent)(IInternalAI * pOuterAI, unsigned ** ppNotifications);

   // Initialize the component
   STDMETHOD_(void, Init)();

   // Clean-up the component
   STDMETHOD_(void, End)();

   // Get the outer AI, note reference count is not incremented
   STDMETHOD_(IInternalAI *, AccessOuterAI)();

   // Notifications
   STDMETHOD_(void, OnSimStart)();
   STDMETHOD_(void, OnSimEnd)();

   STDMETHOD_(void, OnBeginFrame)();
   STDMETHOD_(void, OnEndFrame)();

   STDMETHOD_(void, OnUpdateProperties)(eAIPropUpdateKind kind);

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnGoalProgress)(const cAIGoal * pGoal);
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionChange)(IAIAction * pPrevious, IAIAction * pAction);

   STDMETHOD_(void, OnAwareness)(ObjID source,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   STDMETHOD_(void, OnHighAlert)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnDeath)(const sDamageMsg * pMsg);

   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);

   STDMETHOD_(void, OnSound)(const sSoundInfo *, const sAISoundType * pType);
   STDMETHOD_(void, OnFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity);

   STDMETHOD_(void, OnFoundBody)(ObjID body);

   STDMETHOD_(void, OnFoundSuspicious)(ObjID suspobj);

   STDMETHOD_(void, OnScriptCommand)(eAIScriptCmd command, void *);

   STDMETHOD_(void, OnProperty)(IProperty *, const sPropertyListenMsg *);

   STDMETHOD_(void, OnDefend)(const sAIDefendPoint *);

   STDMETHOD_(void, OnSignal)(const sAISignal *);

   STDMETHOD_(void, OnWitnessCombat)();

   STDMETHOD_(void, OnGoalsReset)(const cAIGoal * pPrevious);

   STDMETHOD_(void, OnCollision)(const sPhysListenMsg *);

   STDMETHOD_(void, OnTripwire)(ObjID objID, BOOL enter);

   // pass in object that is frustrating us.
   STDMETHOD_(void, OnFrustration)(ObjID source, ObjID dest, IAIPath *pPath);

   // Special callbacks
   STDMETHOD_(void, OnGameEvent)(void *);

   // Save/load
   STDMETHOD_(BOOL, PreSave)();
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, PostSave)();

   STDMETHOD_(BOOL, PreLoad)();
   STDMETHOD_(BOOL, Load)(ITagFile *);
   STDMETHOD_(BOOL, PostLoad)();

protected:
   // Notification flag accessors
   unsigned GetNotifications()
   {
      return m_notifications;
   }

   void SetNotifications(unsigned fNotifications);
   void ClearNotifications(unsigned fNotifications);

   // Save load: only needed if component changes notifications on the fly
   void SaveNotifications(ITagFile *);
   void LoadNotifications(ITagFile *);

   // Often used state accessors
   ObjID GetID() const
   {
      return m_pAIState->GetID();
   }

   // Pointers to outer AI common to all components
   IInternalAI *    m_pAI;
   const cAIState * m_pAIState;

private:
   unsigned m_notifications;

};

///////////////////////////////////////////////////////////////////////////////
//
// Base class for generic components
//

class cAIComponent : public cAIComponentBase<IAIComponent, &IID_IAIComponent>
{
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIBASCMP_H */
