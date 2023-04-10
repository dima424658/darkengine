/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aifreeze.cpp,v 1.10 1999/06/09 21:37:23 JON Exp $
//
//
//

#include <aifreeze.h>

#include <appagg.h>
#include <lg.h>
#include <mprintf.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aiactseq.h>
#include <aiactori.h>
#include <aiapibhv.h>
#include <aidfndpt.h>
#include <aigests.h>
#include <aigoal.h>
#include <aiidle.h>
#include <aiprabil.h>

#include <speech.h>

// property stuff
#include <propface.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

// act-react
#include <frezreac.h>

// creature interface
#include <creatext.h>

// scripting
#include <scrptbas.h>
#include <scrptapi.h>

// Must be last header
#include <dbmem.h>

struct sAIFrozen
{
   tSimTime m_startTime;
   tSimTime m_duration;
};

///////////////////////////////////////

static sFieldDesc _g_AIFrozenFieldDesc[] =
{
   { "StartTime", kFieldTypeInt,  FieldLocation(sAIFrozen, m_startTime), },
   { "Duration",   kFieldTypeInt,  FieldLocation(sAIFrozen, m_duration), },
};

static sStructDesc _g_AIFrozenStructDesc = StructDescBuild(sAIFrozen, kStructFlagNone, _g_AIFrozenFieldDesc);

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIFrozenProperty
DECLARE_PROPERTY_INTERFACE(IAIFrozenProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIFrozen *);
};

///////////////////

#define AI_PROP_FROZEN "AI_Frozen"

static sPropertyDesc _g_FrozenPropertyDesc = 
{
   AI_PROP_FROZEN,
   kPropertyInstantiate,
   NULL, 0, 0,  // constraints, version
   { AI_STATE_CAT, "Frozen" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAIFrozenProperty, &IID_IAIFrozenProperty, sAIFrozen*, cHashPropertyStore< cClassDataOps<sAIFrozen> > > cAIFrozenPropertyBase;

class cAIFrozenProperty : public cAIFrozenPropertyBase
{
public:
   cAIFrozenProperty()
      : cAIFrozenPropertyBase(&_g_FrozenPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIFrozen);
};

///////////////////

static IAIFrozenProperty* _g_pAIFrozenProperty;

void AIClearFrozen(ObjID objID)
{
   _g_pAIFrozenProperty->Delete(objID);
}

void AISetFrozen(ObjID obj, tSimTime duration)
{
   sAIFrozen frozen;
   frozen.m_startTime = GetSimTime();
   frozen.m_duration = duration;
   _g_pAIFrozenProperty->Set(obj, &frozen);
}

BOOL AIIsFrozen(ObjID obj)
{
   sAIFrozen *pFrozen;

   if (_g_pAIFrozenProperty->Get(obj, &pFrozen))
   {
      tSimTime time = GetSimTime();
      return ((time>=pFrozen->m_startTime) && ((pFrozen->m_duration<0) || (time-pFrozen->m_startTime<pFrozen->m_duration)));
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

void AIInitFreezeAbility(IAIManager *pAIManager)
{
   StructDescRegister(&_g_AIFrozenStructDesc);
   _g_pAIFrozenProperty = new cAIFrozenProperty;
   _g_pAIFrozenProperty->GetID();
   pAIManager->ListenToProperty(_g_pAIFrozenProperty);
   // should this be here? if not, where?
   FreezeReactionsInit();
}

void AITermFreezeAbility(void)
{
   SafeRelease(_g_pAIFrozenProperty);
   FreezeReactionsTerm();
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFreeze
//

cAIFreeze::cAIFreeze()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIFreeze::GetName()
{
   return "Freeze ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFreeze::Init()
{
   cAIAbility::Init();
   SetNotifications(kAICN_Property);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFreeze::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAIAbility::OnGoalChange(pPrevious, pGoal);
   AutoAppIPtr(ScriptMan);
   if (IsLosingControl(pPrevious, pGoal))
   {
      CreatureUnFreeze(m_pAIState->GetID());
      sScrMsg* pMsg = new sScrMsg(m_pAIState->GetID(), "Unfreeze"); 
      pMsg->flags |= kSMF_MsgPostToOwner;
      pScriptMan->PostMessage(pMsg); 
      ClearNotifications(kAICN_GoalChange);
   }
   else if (IsGainingControl(pPrevious, pGoal))
   {
      CreatureFreeze(m_pAIState->GetID());
      sScrMsg* pMsg = new sScrMsg(m_pAIState->GetID(), "Freeze"); 
      pMsg->flags |= kSMF_MsgPostToOwner;
      pScriptMan->PostMessage(pMsg); 
      SpeechHalt(m_pAIState->GetID());
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFreeze::OnProperty(IProperty *pProp, const sPropertyListenMsg *pMsg)
{
//   IAIFrozenProperty* pAIFrozenProperty;

//   if (FAILED(pProp->QueryInterface(IID_IAIFrozenProperty, (void**)&pAIFrozenProperty)))
   _g_pAIFrozenProperty->GetID();
   if (_g_pAIFrozenProperty->GetID() != pMsg->property)
      return;
   SignalGoal();   

}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIFreeze::UpdateGoal(cAIGoal * pGoal)
{
   if (!AIIsFrozen(m_pAIState->GetID()))
      SignalGoal();
   return pGoal->result;
}

///////////////////////////////////////

STDMETHODIMP cAIFreeze::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (!AIIsFrozen(m_pAIState->GetID()))
   {
      *ppGoal = NULL;
      return S_FALSE;
   }
   // this doesn't really make sense, but it's high priority...
   *ppGoal = new cAIFleeGoal(this);
   (*ppGoal)->priority = kAIP_VeryHigh;
   SetNotifications(kAICN_GoalChange);

   SignalAction();
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIFreeze::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIMotionAction *pAction = new cAIMotionAction(this);
   pNew->Append(pAction);
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
