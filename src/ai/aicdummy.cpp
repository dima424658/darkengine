/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicdummy.cpp,v 1.13 2000/02/11 18:27:33 bfarquha Exp $
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <mtagvals.h>

#include <aiactmot.h>
#include <aiapibhv.h>
#include <aibasabl.h>
#include <aigoal.h>
#include <aiutils.h>
#include <aiactloc.h>
#include <aicdummy.h>

#include <weaphalo.h>
#include <weapon.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <playrobj.h>

// Must be last header
#include <dbmem.h>

/////////////////
// create needed properties

static ILabelProperty *g_pPropCDummyTagA      = NULL;
static ILabelProperty *g_pPropCDummyTagB      = NULL;
static ILabelProperty *g_pPropCDummyTagC      = NULL;
static IIntProperty   *g_pPropCDummyTagCValue = NULL;
static IBoolProperty  *g_pDownWithOBB         = NULL;

static sPropertyDesc DummyTagADesc =      { "TagA_CombDummy", 0, NULL, 0, 0, { "AI Debug", "CombatDummyA" }, kPropertyChangeLocally };
static sPropertyDesc DummyTagBDesc =      { "TagB_CombDummy", 0, NULL, 0, 0, { "AI Debug", "CombatDummyB" }, kPropertyChangeLocally };
static sPropertyDesc DummyTagCDesc =      { "TagC_CombDummy", 0, NULL, 0, 0, { "AI Debug", "CombatDummyC" }, kPropertyChangeLocally };
static sPropertyDesc DummyTagCValueDesc = { "TagCVal_CDummy", 0, NULL, 0, 0, { "AI Debug", "CDummyCValue" }, kPropertyChangeLocally };
static sPropertyDesc DummyTagBlockTest =  { "Block Test",     0, NULL, 0, 0, { "AI Debug", "BlockTest" }, kPropertyChangeLocally };

BOOL AIInitCombatDummyAbility(IAIManager *)
{
#if 0
   g_pPropCDummyTagA=CreateLabelProperty(&DummyTagADesc,kPropertyImplVerySparse);
   g_pPropCDummyTagB=CreateLabelProperty(&DummyTagBDesc,kPropertyImplVerySparse);
   g_pPropCDummyTagC=CreateLabelProperty(&DummyTagCDesc,kPropertyImplVerySparse);
   g_pPropCDummyTagCValue=CreateIntProperty(&DummyTagCValueDesc,kPropertyImplVerySparse);
   g_pDownWithOBB=CreateBoolProperty(&DummyTagBlockTest,kPropertyImplVerySparse);
#endif
   return TRUE;
}

BOOL AITermCombatDummyAbility(void)
{
#if 0
   SafeRelease(g_pPropCDummyTagA);
   SafeRelease(g_pPropCDummyTagB);
   SafeRelease(g_pPropCDummyTagC);
   SafeRelease(g_pPropCDummyTagCValue);
   SafeRelease(g_pDownWithOBB);
#endif
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CLASS: cAICombatDummy
///////////////////////////////////////

STDMETHODIMP_(const char *) cAICombatDummy::GetName()
{
   return "Combat Dummy ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICombatDummy::Init()
{
   cAINonCombatAbility::Init();
   m_BlockCount=0;
   m_BlockDirection=-1;  // no direction
   m_IHateEveryone=0;
}

///////////////////////////////////////

// see if it is time for a new action

DECLARE_TIMER(cAICombatDummy_OnActionProgress, Average);

STDMETHODIMP_(void) cAICombatDummy::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAICombatDummy_OnActionProgress);

// If we're in a good position to interrupt...
   if (pAction->GetResult() > kAIR_NoResult)
      if (IsOwn(pAction) && !pAction->InProgress())
         SignalAction();  // if we completed our goal, signal for more
}

STDMETHODIMP_(void) cAICombatDummy::OnGameEvent(void *magic)
{
   m_BlockDirection=(int)magic;
   SignalAction();
}

// SUICIDE IS THE ONLY HOPE
STDMETHODIMP_(void) cAICombatDummy::OnBeginFrame(void)
{
   if (m_IHateEveryone)
      SignalAction();
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAICombatDummy::DummyTagsAvailable(void)
{
   if (g_pDownWithOBB->IsRelevant(GetID()))
      return TRUE;
   if (g_pPropCDummyTagA->IsRelevant(GetID()))
      return TRUE;
   if (g_pPropCDummyTagB->IsRelevant(GetID()))
      return TRUE;
   if (g_pPropCDummyTagC->IsRelevant(GetID()))
      return TRUE;
   return FALSE;
}

STDMETHODIMP cAICombatDummy::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (!DummyTagsAvailable())
   {
      *ppGoal = NULL;
      return S_FALSE;
   }

   *ppGoal = new cAIAttackGoal(this);
   (*ppGoal)->priority = kAIP_Absolute;
   (*ppGoal)->object = GetID();
   (*ppGoal)->LocFromObj();
   SetNotifications(kAICN_ActionProgress);
   SetNotifications(kAICN_GoalChange);

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAICombatDummy::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   Label *tag_str;
   cAIMotionAction * pAction = CreateMotionAction();

   if (g_pDownWithOBB->IsRelevant(GetID()))
   {  // lets do our secret blocking wackiness!!!
      if (m_IHateEveryone)
      {
         int i;
         m_IHateEveryone=0;
         ClearNotifications(kAICN_BeginFrame);
//         WeaponHaloInflate(GetID(),GetWeaponObjID(GetID()));
         *pNew=previous;
         for (i=0; i<pNew->Size(); i++)
            (*pNew)[i]->AddRef();
         return S_OK;
      }
      if (m_BlockDirection>=0)
      {  // YO! we must BloCK! dope
         enum eMTagDirectionValues dir_tags[]={kMTV_high,kMTV_right,kMTV_low,kMTV_left};
         pAction->AddTags(cTag("MeleeCombat",kMTV_set));
         pAction->AddTags(cTag("WithSword",kMTV_set));
         pAction->AddTags(cTag("Block",kMTV_set));
         if (m_BlockDirection<(sizeof(dir_tags)/sizeof(dir_tags[0])))
            pAction->AddTags(cTag("Direction",dir_tags[m_BlockDirection]));
         m_BlockDirection=-1;
         m_BlockCount=-1;
         ClearNotifications(kAICN_GameEvent);
         pNew->Append(pAction);
         return S_OK;
      }
      BOOL is_blocking=FALSE;
      if (m_BlockCount>0) // we are mid blocking idle wackiness
         if (++m_BlockCount>6)
         {
//            WeaponHaloDeflate(GetID(),GetWeaponObjID(GetID()));
            mprintf("Yo, block off\n");
            m_BlockCount=-1; // reset to not blocking
            ClearNotifications(kAICN_GameEvent);
         }
         else
            is_blocking=TRUE;
      else
         if (--m_BlockCount<-3)
         {  // reset to blocking
            mprintf("Yo, lets block\n");
            // WeaponHaloInflate(GetID(),GetWeaponObjID(GetID()));
            SetNotifications(kAICN_GameEvent);
            is_blocking=TRUE;
            m_BlockCount=1;
         }  // else normal idle
      if (is_blocking)
      {
         m_IHateEveryone=1;
         SetNotifications(kAICN_BeginFrame);
         pAction->AddTags(cTag("Block",kMTV_set));  // someday, TODAY!!! HA HA make this block?
         pAction->SetFocus(PlayerObject());  //! @TBD: multiplayer? (mtr)
      }
      else
         pAction->AddTags(cTag("Stand",kMTV_set));
      pAction->AddTags(cTag("MeleeCombat",kMTV_set));
      pAction->AddTags(cTag("WithSword",kMTV_set));
      pNew->Append(pAction);
      return S_OK;
   }
   // otherwise run normal combat dummy
   pAction->AddTags(cTag("MeleeCombat",kMTV_set));
   if (g_pPropCDummyTagA->Get(GetID(),&tag_str))
      pAction->AddTags(cTag(tag_str->text,kMTV_set));
   if (g_pPropCDummyTagB->Get(GetID(),&tag_str))
      pAction->AddTags(cTag(tag_str->text,kMTV_set));
   if (g_pPropCDummyTagC->Get(GetID(),&tag_str))
   {
      int val=kMTV_set;
      g_pPropCDummyTagCValue->Get(GetID(),&val);
      pAction->AddTags(cTag(tag_str->text,val));
   }
   pNew->Append(pAction);
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
