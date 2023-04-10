/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasact.cpp,v 1.14 2000/02/11 18:27:26 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <aiapibhv.h>
#include <aiapiiai.h>
#include <aibasact.h>
#include <aitagtyp.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Goal Type Enum to String
//

static const char * g_ppszActionTypes[] =
{
   "Null",
   "Wait",
   "Move",                                       // kAIAT_Move
   "Locomote",                                   // kAIAT_Locomote
   "Motion",                                     // kAIAT_Motion
   "Sound",                                      // kAIAT_Sound
   "Orient",                                     // kAIAT_Orient
   "Stand",                                      // kAIAT_Stand

   "Sequenced action",                           // kAIAT_Sequence
   "Frob",                                       // kAIAT_Frob
   "Follow",                                     // kAIAT_Follow
   "Investigate",                                // kAIAT_Investigate
   "Wander",                                     // kAIAT_Wander
   "Move to a marker",                           // kAIAT_MoveToMarker
   "Psuedo-script",                              // kAIAT_PsuedoScript
   "Proxy",

   "Attack - ranged",                            // kAIAT_AttackRanged
   "Launch a projectile"                         // kAIAT_Launch
};

static cStr g_CustomStr;

///////////////////////////////////////

const char * AIGetActTypeName(tAIActionType type)
{
   AssertMsg(type < kAIAT_LastAction || type >= kAIAT_CustomBase, "Invalid action type");
   if (type <= kAIAT_CustomBase)
      return g_ppszActionTypes[type];
   g_CustomStr.FmtStr("Custom 0x%x", type);
   return g_CustomStr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAction
//

STDMETHODIMP_(void) cAIAction::Describe(cStr * pStr)
{
   pStr->FmtStr("'%s' action", AIGetActTypeName(type));
   if (tags.Size())
   {
      cStr tagString;

      tags.ToString(&tagString);
      *pStr += " with tags '";
      *pStr += tagString;
      *pStr += "'";
   }
}

///////////////////////////////////////
//
// Access the internals of the action
//

STDMETHODIMP_(BOOL) cAIAction::InProgress()
{
   return AIInProgress(result);
}

///////////////////////////////////////

STDMETHODIMP_(tAIActionType) cAIAction::GetType()
{
   return type;
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIAction::GetResult()
{
   return result;

///////////////////////////////////////
}

STDMETHODIMP_(DWORD) cAIAction::GetData(unsigned index)
{
   return ownerDataSet[index];
}

///////////////////////////////////////

STDMETHODIMP_(const sAIAction *) cAIAction::Access()
{
   return this;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAction::IsProxy()
{
   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(IAIAction *) cAIAction::GetInnerAction()
{
   return NULL;
}

///////////////////////////////////////

STDMETHODIMP_(IAIAction *) cAIAction::GetTrueAction()
{
   return this;
}

///////////////////////////////////////

STDMETHODIMP_(IAIActor *) cAIAction::GetTrueOwner()
{
   return pOwner;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAction::Started()
{
   return (flags & kAIAF_Started);
}

///////////////////////////////////////
//
// Update the action
//

STDMETHODIMP_(eAIResult) cAIAction::Update()
{
   return (result = kAIR_NoResultSwitch);
}

///////////////////////////////////////
//
//
//

STDMETHODIMP_(eAIResult) cAIAction::Enact(ulong deltaTime)
{
   return (result = kAIR_NoResultSwitch);
}

///////////////////////////////////////
//
// Terminate the action
//

DECLARE_TIMER(cAIAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIAction::End()
{
   AUTO_TIMER(cAIAction_Enact);

   if (result < kAIR_Success)
      result = kAIR_Interrupted;
   return result;
}

///////////////////////////////////////

BOOL cAIAction::CanEnact()
{
   return (result == kAIR_NoResultSwitch || !Started());
}

///////////////////////////////////////

cAIMoveAction * cAIAction::CreateMoveAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateMoveAction(pOwner, data);
}

///////////////////////////////////////

cAILocoAction * cAIAction::CreateLocoAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateLocoAction(pOwner, data);
}

///////////////////////////////////////

cAIMotionAction * cAIAction::CreateMotionAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateMotionAction(pOwner, data);
}

///////////////////////////////////////

cAISoundAction * cAIAction::CreateSoundAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateSoundAction(pOwner, data);
}

///////////////////////////////////////

cAIOrientAction * cAIAction::CreateOrientAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateOrientAction(pOwner, data);
}

///////////////////////////////////////

cAIFrobAction * cAIAction::CreateFrobAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateFrobAction(pOwner, data);
}

///////////////////////////////////////

cAIFollowAction * cAIAction::CreateFollowAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateFollowAction(pOwner, data);
}

///////////////////////////////////////

cAIInvestAction * cAIAction::CreateInvestAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateInvestAction(pOwner, data);
}

///////////////////////////////////////

cAIWanderAction * cAIAction::CreateWanderAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateWanderAction(pOwner, data);
}

///////////////////////////////////////

cAIPsdScrAction * cAIAction::CreatePsdScrAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreatePsdScrAction(pOwner, data);
}

///////////////////////////////////////

cAIWaitAction * cAIAction::CreateWaitAction(DWORD data)
{
   return m_pAI->AccessBehaviorSet()->CreateWaitAction(pOwner, data);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAction::Save(ITagFile * pTagFile)
{
   // Don't save anything unless the derived class knows how to save.
   return TRUE;
}

///////////////////////////////////////


STDMETHODIMP_(BOOL) cAIAction::Load(ITagFile * pTagFile)
{
   // Don't load anything unless the derived class knows how to load.
   return FALSE;  // don't use this action.
}

///////////////////////////////////////

void cAIAction::SaveActionBase(ITagFile * pTagFile)
{
   cStr tagString;
   tags.ToString(&tagString);
   AITagMoveString(pTagFile, &tagString);
   AITagMove(pTagFile, &flags);
   AITagMove(pTagFile, &time);
   AITagMove(pTagFile, &pctComplete);
   AITagMove(pTagFile, &result);
}

///////////////////////////////////////

void cAIAction::LoadActionBase(ITagFile * pTagFile)
{
   cStr tagString;
   AITagMoveString(pTagFile, &tagString);
   tags.FromString(tagString);
   AITagMove(pTagFile, &flags);
   AITagMove(pTagFile, &time);
   AITagMove(pTagFile, &pctComplete);
   AITagMove(pTagFile, &result);
}

///////////////////////////////////////////////////////////////////////////////



STDMETHODIMP_(IAIPath *) cAIAction::GetPath()
{
   return 0;
}

