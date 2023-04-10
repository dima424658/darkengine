/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasbhv.cpp,v 1.56 2000/01/24 20:14:22 adurant Exp $
//
//
//

#include <lg.h>

#include <ai.h>
#include <aibasabl.h>
#include <aibasact.h>
#include <aibasbhv.h>
#include <aibascmp.h>
#include <aibasctm.h>
#include <aibasmrg.h>
#include <aibassns.h>
#include <aiactfol.h>
#include <aiactfrb.h>
#include <aiactinv.h>
#include <aiactloc.h>
#include <aiactlch.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aiactori.h>
#include <aiactps.h>
#include <aiactsnd.h>
#include <aiacttyp.h>
#include <aiactwnd.h>
#include <aiactwt.h>
#include <aiactjr.h>
#include <aiactjs.h>
#include <aiactjsc.h>
#include <aicbhtoh.h>
//#include <aicbrngd.h>
#include <aicbrnew.h>
#include <aicombat.h>
#include <aidbgcmp.h>
#include <aideath.h>
#include <aidefend.h>
#include <aifollow.h>
#include <aigests.h>
#include <aimove.h>
#include <aiidle.h>
#include <aiinvest.h>
#include <aipatrol.h>
#include <aiscrabl.h>
#include <aisound.h>
#include <aistdmrg.h>
#include <aitest.h>
#include <aicdummy.h>
#include <aistun.h>
#include <aidetect.h>
#include <aiflee.h>
#include <aifreeze.h>
#include <aiwatch.h>
#include <ailisten.h>
#include <aithreat.h>
#include <aialtrsp.h>
#include <aimplyr.h>
#include <aimttest.h>
#include <aidoormr.h>
#include <aincbdmg.h>
#include <aiinform.h>
#include <aicbtrsp.h>
#include <aicnvabl.h>
#include <aipthfnd.h>
#include <ainet.h>
#include <aibdyrsp.h>
#include <aisusrsp.h>
#include <aiproxy.h>

// Must be last header
#include <dbmem.h>

//#define TEST_ABILITY 1

///////////////////////////////////////////////////////////////////////////////
//
// Gestures to Motions Table
//

struct sAIGestureMotionEntry
{
   sAIGestureMotionEntry(eAIGestureType _type, const char * _pszName, const char * _tags)
    : type(_type), pszName(_pszName), tags(_tags) {}

   eAIGestureType type;
   const char *   pszName;
   cTagSet        tags;
};

////////////////////////////////////

#define GESTMOT(e, string) { sAIGestureMotionEntry(e, #e, string) }

static sAIGestureMotionEntry g_AIGestureMotionTable[] =
{
   GESTMOT( kAIGT_None,          ""                ),
   GESTMOT( kAIGT_Calibration,   ""                ),
   GESTMOT( kAIGT_Rest,          ""                ),
   GESTMOT( kAIGT_Fidget,        "IdleGesture 0"   )
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBehaviorSet
//

////////////////////////////////////

cAIBehaviorSet::cAIBehaviorSet()
{

}

////////////////////////////////////

cAIBehaviorSet::~cAIBehaviorSet()
{

}

////////////////////////////////////
//
// Find out the behavior set name
//

STDMETHODIMP_(const char *) cAIBehaviorSet::GetName()
{
   return "Default";
}

////////////////////////////////////
//
// Create an AI object for the creature type
//

STDMETHODIMP_(cAI *) cAIBehaviorSet::CreateAI(ObjID id)
{
   cAI * pAI = new cAI;
   if (pAI->Init(id, this) != S_OK)
      SafeRelease(pAI);
   return pAI;
}

////////////////////////////////////

void cAIBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
{
   // Debugging/development tools
#ifndef SHIP
   pComponents->Append(new cAIFlowDebugger);
#endif

#ifdef TEST_ABILITY
   pComponents->Append(new cAITest);
#endif

   // Enactors
   pComponents->Append(new cAIMoveEnactor);
   pComponents->Append(new cAISoundEnactor);

   // Pathfinder
   pComponents->Append(new cAIPathfinder);

   // Movement regulators
   pComponents->Append(new cAIObjectsMovReg);
   pComponents->Append(new cAIWallsCliffsMovReg);
   pComponents->Append(new cAIDoorMovReg);
   pComponents->Append(new cAILVLMovReg);

   // Senses
   pComponents->Append(new cAISenses);

   // Defend point watcher
   pComponents->Append(new cAIDefend);

#ifdef NEW_NETWORK_ENABLED
   // Networking interface
   pComponents->Append(new cAINetwork);
#endif
}

////////////////////////////////////

void cAIBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIDeath);
   pComponents->Append(new cAIBasicScript);
   pComponents->Append(new cAIStun);
   pComponents->Append(new cAIDetect);
}

////////////////////////////////////

void cAIBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIInvestigate);
   pComponents->Append(new cAIPatrol);
   pComponents->Append(new cAIIdle);
   pComponents->Append(new cAIFlee);
   pComponents->Append(new cAINonCombatDamageResponse);
   pComponents->Append(new cAIFollow);
   pComponents->Append(new cAISignalResponse);
   pComponents->Append(new cAIAlertResponse);
   pComponents->Append(new cAIBodyResponse);
   pComponents->Append(new cAIThreatResponse);
   pComponents->Append(new cAIWatch);
   pComponents->Append(new cAIConverse);
   pComponents->Append(new cAIInform);
   pComponents->Append(new cAISenseCombatResponse);
   pComponents->Append(new cAISuspiciousResponse);
   pComponents->Append(new cAIFreeze);
}

////////////////////////////////////

void cAIBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICombatHtoH);

#if 0
   pComponents->Append(new cAICombatRanged);
   pComponents->Append(new cAICombatDummy);
#endif
}

////////////////////////////////////
//
// Create the set of components that define the AIs potential
//

STDMETHODIMP cAIBehaviorSet::CreateComponents(cAIComponentPtrs * pComponents)
{
   CreateNonAbilityComponents(pComponents);

#ifndef TEST_ABILITY
   CreateGenericAbilities(pComponents);
   CreateNonCombatAbilities(pComponents);
   CreateCombatAbilities(pComponents);
#endif

   return S_OK;
}

////////////////////////////////////
//
// Create a stock action object of a specified type
//

STDMETHODIMP_(cAIMoveAction *) cAIBehaviorSet::CreateMoveAction(IAIActor * pOwner, DWORD data)
{
   return new cAIMoveAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAILocoAction *) cAIBehaviorSet::CreateLocoAction(IAIActor * pOwner, DWORD data)
{
   return new cAILocoAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIMotionAction *) cAIBehaviorSet::CreateMotionAction(IAIActor * pOwner, DWORD data)
{
   return new cAIMotionAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAISoundAction *) cAIBehaviorSet::CreateSoundAction(IAIActor * pOwner, DWORD data)
{
   return new cAISoundAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIOrientAction *) cAIBehaviorSet::CreateOrientAction(IAIActor * pOwner, DWORD data)
{
   return new cAIOrientAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIFrobAction *) cAIBehaviorSet::CreateFrobAction(IAIActor * pOwner, DWORD data)
{
   return new cAIFrobAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIFollowAction *) cAIBehaviorSet::CreateFollowAction(IAIActor * pOwner, DWORD data)
{
   return new cAIFollowAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIInvestAction *) cAIBehaviorSet::CreateInvestAction(IAIActor * pOwner, DWORD data)
{
   return new cAIInvestAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIWanderAction *) cAIBehaviorSet::CreateWanderAction(IAIActor * pOwner, DWORD data)
{
   return new cAIWanderAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIPsdScrAction *) cAIBehaviorSet::CreatePsdScrAction(IAIActor * pOwner, DWORD data)
{
   return new cAIPsdScrAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIWaitAction *) cAIBehaviorSet::CreateWaitAction(IAIActor * pOwner, DWORD data)
{
   return new cAIWaitAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAILaunchAction *) cAIBehaviorSet::CreateLaunchAction(IAIActor * pOwner, DWORD data)
{
   return new cAILaunchAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIJointRotateAction *) cAIBehaviorSet::CreateJointRotateAction(IAIActor * pOwner, DWORD data)
{
   return new cAIJointRotateAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIJointSlideAction *) cAIBehaviorSet::CreateJointSlideAction(IAIActor * pOwner, DWORD data)
{
   return new cAIJointSlideAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIJointScanAction *) cAIBehaviorSet::CreateJointScanAction(IAIActor * pOwner, DWORD data)
{
   return new cAIJointScanAction(pOwner, data);
}

////////////////////////////////////

STDMETHODIMP_(cAIAction *) cAIBehaviorSet::CreateGestureAction(eAIGestureType gesture, IAIActor * pOwner, DWORD data)
{
   IInternalAI *    pAI      = pOwner->AccessOuterAI();
   IAIMoveEnactor * pEnactor = pAI->AccessMoveEnactor();

   if (pEnactor)
   {
      if (pEnactor->SupportsMotionActions())
      {
         for (int i = 0; i < sizeof(g_AIGestureMotionTable); i++)
         {
            if (g_AIGestureMotionTable[i].type == gesture)
            {
               cAIMotionAction * pAction = CreateMotionAction(pOwner, data);
               pAction->AddTags(g_AIGestureMotionTable[i].tags);
               return pAction;
            }
         }
      }
      else
      {

      }
   }

   return new cAINullAction(pOwner, data);
}

////////////////////////////////////
//
// Create a game-specific action object
//

STDMETHODIMP_(cAIAction *) cAIBehaviorSet::CreateCustomAction(tAIActionType type, IAIActor * pOwner, DWORD data)
{
   CriticalMsg("There are no default custom actions");
   return new cAINullAction(pOwner, data);
}

/////////////////////////////////////
//
// Factories for actions that may have proxies in network games.
//

STDMETHODIMP_(void) cAIBehaviorSet::EnactProxyLaunchAction(IAI *pAI, void *netmsg)
{
   cAILaunchAction::EnactFire(pAI, netmsg);
}

////////////////////////////////////

STDMETHODIMP_(void) cAIBehaviorSet::EnactProxyJointRotateAction(IAI *pAI, void *netmsg)
{
   new cAIJointRotateAction(pAI, netmsg);
}

////////////////////////////////////

STDMETHODIMP_(void) cAIBehaviorSet::EnactProxyJointSlideAction(IAI *pAI, void *netmsg)
{
   new cAIJointSlideAction(pAI, netmsg);
}

////////////////////////////////////

STDMETHODIMP_(void) cAIBehaviorSet::EnactProxyJointScanAction(IAI *pAI, void *netmsg)
{
   cAIJointScanAction::HandleNetMessage(pAI, netmsg);
}

////////////////////////////////////

STDMETHODIMP_(void) cAIBehaviorSet::EnactProxyCustomAction(tAIActionType type, IAI *pAI,
                                                           void *netmsg)
{
   CriticalMsg("There are no default custom proxy actions");
}

//////////////////////////////////////
//
// Create an action of the given type.
//
cAIAction *cAIBehaviorSet::CreateAnyAction(tAIActionType type, IAIActor *pOwner, DWORD data)
{
   switch (type)
   {
      case kAIAT_Null:         return NULL;
      case kAIAT_Wait:         return CreateWaitAction(pOwner, data);
      case kAIAT_Move:         return CreateMoveAction(pOwner, data);
      case kAIAT_Locomote:     return CreateLocoAction(pOwner, data);
      case kAIAT_Motion:       return CreateMotionAction(pOwner, data);
      case kAIAT_Sound:        return CreateSoundAction(pOwner, data);
      case kAIAT_Orient:       return CreateOrientAction(pOwner, data);
      case kAIAT_Stand:        return NULL;
      case kAIAT_Sequence:     return NULL;
      case kAIAT_Frob:         return CreateFrobAction(pOwner, data);
      case kAIAT_Follow:       return CreateFollowAction(pOwner, data);
      case kAIAT_Investigate:  return CreateInvestAction(pOwner, data);
      case kAIAT_Wander:       return CreateWanderAction(pOwner, data);
      case kAIAT_MoveToMarker: return NULL;
      case kAIAT_PsuedoScript: return CreatePsdScrAction(pOwner, data);
      case kAIAT_AttackRanged: return NULL;
      case kAIAT_Launch:       return NULL;
      default:
         if (type >= kAIAT_CustomBase)
         {
            return NULL; // @TBD (toml 11-04-98): CreateCustomAction(type, pOwner, data);
         }
         else
         {
//            Warning(("CreateAnyAction:  Unknown action type %d\n", type));
            Warning(("CreateAnyAction:  Unknown action type %s\n", AIGetActTypeName(type)));
            return NULL; //new cAINullAction(pOwner, data);
         }
   }
}

/////////////////////////////////

STDMETHODIMP_(void) cAIBehaviorSet::SaveAction(ITagFile * pTagFile, IAIAction *pAction)
{
   tAIActionType actionType = pAction->GetType();
   AITagMove(pTagFile, &actionType);
   DWORD data = pAction->GetData();
   AITagMove(pTagFile, &data);
   pAction->Save(pTagFile);
}

/////////////////////////////////

STDMETHODIMP_(cAIAction *) cAIBehaviorSet::LoadAndCreateAction(ITagFile *pTagFile, IAIActor *pOwner)
{
   tAIActionType actionType;
   DWORD data;

   AITagMove(pTagFile, &actionType);
   AITagMove(pTagFile, &data);

   // Create an action of this type
   cAIAction *pAction = CreateAnyAction(actionType, pOwner, data);

   // Load it (then delete it if it failed to load).
   if (pAction && !pAction->Load(pTagFile))
      SafeRelease(pAction);

   return pAction;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionTestBehaviorSet
//

cAIMotionTestBehaviorSet::cAIMotionTestBehaviorSet()
{

}

////////////////////////////////////

cAIMotionTestBehaviorSet::~cAIMotionTestBehaviorSet()
{

}

////////////////////////////////////

STDMETHODIMP_(const char *) cAIMotionTestBehaviorSet::GetName()
{
   return "MotionTest";
}

////////////////////////////////////

void cAIMotionTestBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAITestMotionPlayer);
}

////////////////////////////////////

void cAIMotionTestBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{

}

////////////////////////////////////

void cAIMotionTestBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{

}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionTagsTestBehaviorSet
//

cAIMotionTagsTestBehaviorSet::cAIMotionTagsTestBehaviorSet()
{

}

////////////////////////////////////

cAIMotionTagsTestBehaviorSet::~cAIMotionTagsTestBehaviorSet()
{

}

////////////////////////////////////

STDMETHODIMP_(const char *) cAIMotionTagsTestBehaviorSet::GetName()
{
   return "MotionTagsTest";
}

////////////////////////////////////

void cAIMotionTagsTestBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIMotionTagsTester);
}

////////////////////////////////////

void cAIMotionTagsTestBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{

}

////////////////////////////////////

void cAIMotionTagsTestBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{

}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIRangedBehaviorSet
//

STDMETHODIMP_(const char *) cAIRangedBehaviorSet::GetName()
{
   return "Ranged combatant";
}

///////////////////////////////////////

void cAIRangedBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
//   pComponents->Append(new cAICombatRanged);
   pComponents->Append(new cAICombatNewRanged);
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISimpleBehaviorSet
//

STDMETHODIMP_(const char *) cAISimpleBehaviorSet::GetName()
{
   return "SimpleNC";
}

///////////////////////////////////////


void cAISimpleBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINetProxyBehaviorSet
//

////////////////////////////////////

STDMETHODIMP_(const char *) cAINetProxyBehaviorSet::GetName()
{
   return "NetProxy";
}

////////////////////////////////////

STDMETHODIMP_(cAI *) cAINetProxyBehaviorSet::CreateAI(ObjID id)
{
   cAI * pAI = new cAIProxy;
   if (pAI->Init(id, this) != S_OK)
      SafeRelease(pAI);
   return pAI;
}

////////////////////////////////////

STDMETHODIMP cAINetProxyBehaviorSet::CreateComponents(cAIComponentPtrs * pComponents)
{
   // The only components needed by a proxy are the motion enactor, and the hand-to-hand
   // combat enactor (which won't be used if it isn't appropriate for this ai).  Most other
   // components are for higher level decision making not done on the proxy machine.
   // The sound actions are broadcast at a lower level, so don't need the sound enactor.

   // Proxies no longer need any real intelligence, and positively shouldn't
   // have a move enactor, since it conflicts with the ghost system...
   // pComponents->Append(new cAIMoveEnactor);
   // pComponents->Append(new cAICombatHtoH);
   // Pathfinder
   // pComponents->Append(new cAIPathfinder);

   // Debugging/development tools
#ifndef SHIP
   pComponents->Append(new cAIFlowDebugger);
#endif
   pComponents->Append(new cAISenses);

   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWatcherBehaviorSet
//

STDMETHODIMP_(const char *) cAIWatcherBehaviorSet::GetName()
{
   return "Watcher";
}

////////////////////////////////////

void cAIWatcherBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
{
   // Debugging/development tools
#ifndef SHIP
   pComponents->Append(new cAIFlowDebugger);
#endif

   // Senses
   pComponents->Append(new cAISenses);
}

////////////////////////////////////

void cAIWatcherBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
}

////////////////////////////////////

void cAIWatcherBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIAlertResponse);
   pComponents->Append(new cAIWatch);
   pComponents->Append(new cAISenseCombatResponse);
}

////////////////////////////////////

void cAIWatcherBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{

}


