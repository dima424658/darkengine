/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasbhv.h,v 1.14 1999/12/01 16:06:06 BFarquha Exp $
//
//
//

#ifndef __AIBASBHV_H
#define __AIBASBHV_H

#include <aiapibhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBehaviorSet
//

class cAIBehaviorSet : public cCTUnaggregated<IAIBehaviorSet, &IID_IAIBehaviorSet, kCTU_Default>
{
public:
   cAIBehaviorSet();
   virtual ~cAIBehaviorSet();


   ////////////////////////////////////
   //
   // AI Manager services
   //

   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

   //
   // Create an AI object for the creature type
   //
   STDMETHOD_(cAI *, CreateAI)(ObjID id);

   ////////////////////////////////////
   //
   // Individual AI services
   //

   //
   // Create the set of components that define the AIs potential
   //
   STDMETHOD (CreateComponents)(cAIComponentPtrs * pComponents);

   //
   // Create a stock action object of a specified type
   //
   STDMETHOD_(cAIMoveAction *,   CreateMoveAction)  (IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAILocoAction *,   CreateLocoAction)  (IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIMotionAction *, CreateMotionAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAISoundAction *,  CreateSoundAction) (IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIOrientAction *, CreateOrientAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIFrobAction *,   CreateFrobAction)  (IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIFollowAction *, CreateFollowAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIInvestAction *, CreateInvestAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIWanderAction *, CreateWanderAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIPsdScrAction *, CreatePsdScrAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIWaitAction *,   CreateWaitAction)  (IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAILaunchAction *, CreateLaunchAction)  (IAIActor * pOwner, DWORD data = 0);

   STDMETHOD_(cAIJointRotateAction *,CreateJointRotateAction)(IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIJointSlideAction *, CreateJointSlideAction) (IAIActor * pOwner, DWORD data = 0);
   STDMETHOD_(cAIJointScanAction *,  CreateJointScanAction)  (IAIActor * pOwner, DWORD data = 0);

   //
   // Create a gesture for an abstract AI concept
   //
   STDMETHOD_(cAIAction *, CreateGestureAction)(eAIGestureType gesture,
                                                IAIActor * pOwner,
                                                DWORD data = 0);


   //
   // Create a game-specific action object
   //
   STDMETHOD_(cAIAction *, CreateCustomAction)(tAIActionType type,
                                               IAIActor *  pOwner,
                                               DWORD         data = 0);

   //
   // Factories for actions that may have proxies in network games.
   // @TBD (toml 02-25-99): should redo terminology as "netprox"
   //
   STDMETHOD_(void, EnactProxyLaunchAction)     (IAI *pAI, void *netmsg);
   STDMETHOD_(void, EnactProxyJointRotateAction)(IAI *pAI, void *netmsg);
   STDMETHOD_(void, EnactProxyJointSlideAction) (IAI *pAI, void *netmsg);
   STDMETHOD_(void, EnactProxyJointScanAction)  (IAI *pAI, void *netmsg);

   STDMETHOD_(void, EnactProxyCustomAction)(tAIActionType type, IAI *pAI, void *netmsg);

   //
   // Save enough information to recreate this action.
   //
   STDMETHOD_(void, SaveAction)(ITagFile * pTagFile, IAIAction *pAction);

   //
   // Load and create an appropiate kind of action based on what is saved in pTagFile.
   //
   STDMETHOD_(cAIAction *, LoadAndCreateAction)(ITagFile *pTagFile, IAIActor *pOwner);

protected:
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);

   // Create an action of the given type.
   virtual cAIAction *CreateAnyAction(tAIActionType type, IAIActor *pOwner, DWORD data);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionTestBehaviorSet
//

class cAIMotionTestBehaviorSet : public cAIBehaviorSet
{
public:
   cAIMotionTestBehaviorSet();
   virtual ~cAIMotionTestBehaviorSet();

   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionTagsTestBehaviorSet
//

class cAIMotionTagsTestBehaviorSet : public cAIBehaviorSet
{
public:
   cAIMotionTagsTestBehaviorSet();
   virtual ~cAIMotionTagsTestBehaviorSet();

   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIRangedBehaviorSet
//

class cAIRangedBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIRangedBehaviorSet
//

class cAISimpleBehaviorSet : public cAIBehaviorSet
{
public:
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};



///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINetProxyBehaviorSet
//

class cAINetProxyBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   STDMETHOD_(cAI *, CreateAI) (ObjID id);

   STDMETHOD (CreateComponents)(cAIComponentPtrs * pComponents);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWatcherBehaviorSet
//

class cAIWatcherBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBASBHV_H */

