/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ai/aiapibhv.h,v 1.9 1999/03/02 17:42:22 TOML Exp $
//
// Factory interface for AI customization
//

#ifndef __AIAPIBHV_H
#define __AIAPIBHV_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <dynarray.h>

#include <aiapi.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIBehaviorSet);
F_DECLARE_INTERFACE(IAIComponent);
F_DECLARE_INTERFACE(IAIAbility);
F_DECLARE_INTERFACE(IAIActor);
F_DECLARE_INTERFACE(IAIAction);
F_DECLARE_INTERFACE(ITagFile);

///////////////////////////////////////

typedef unsigned eAIGestureType;
typedef unsigned tAIActionType;

///////////////////////////////////////

class cAI;

class cAIAction;
class cAIMoveAction;
class cAILocoAction;
class cAIMotionAction;
class cAISoundAction;
class cAIOrientAction;
class cAIFrobAction;
class cAIFollowAction;
class cAIInvestAction;
class cAIWanderAction;
class cAIPsdScrAction;
class cAIWaitAction;
class cAILaunchAction;
class cAIJointRotateAction;
class cAIJointSlideAction;
class cAIJointScanAction;

///////////////////////////////////////

typedef cDynArray<IAIComponent *> cAIComponentPtrs;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIBehaviorSet
//
// The heart of AI behavioral distinction.
//

DECLARE_INTERFACE_(IAIBehaviorSet, IUnknown)
{
   ////////////////////////////////////
   //
   // AI Manager services
   //

   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)() PURE;

   //
   // Create an AI object for the creature type
   //
   STDMETHOD_(cAI *, CreateAI)(ObjID id) PURE;

   ////////////////////////////////////
   //
   // Individual AI services
   //

   //
   // Create the set of components that define the AIs potential
   //
   STDMETHOD (CreateComponents)(cAIComponentPtrs * pComponents) PURE;

   //
   // Create a stock action object of a specified type
   //
   STDMETHOD_(cAIMoveAction *,   CreateMoveAction)  (IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAILocoAction *,   CreateLocoAction)  (IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIMotionAction *, CreateMotionAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAISoundAction *,  CreateSoundAction) (IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIOrientAction *, CreateOrientAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIFrobAction *,   CreateFrobAction)  (IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIFollowAction *, CreateFollowAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIInvestAction *, CreateInvestAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIWanderAction *, CreateWanderAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIPsdScrAction *, CreatePsdScrAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIWaitAction *,   CreateWaitAction)  (IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAILaunchAction *, CreateLaunchAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIJointRotateAction *,CreateJointRotateAction)(IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIJointSlideAction *, CreateJointSlideAction) (IAIActor * pOwner, DWORD data = 0) PURE;
   STDMETHOD_(cAIJointScanAction *,  CreateJointScanAction)  (IAIActor * pOwner, DWORD data = 0) PURE;

   //
   // Create a gesture for an abstract AI concept
   //
   STDMETHOD_(cAIAction *, CreateGestureAction)(eAIGestureType gesture, 
                                                IAIActor *   pOwner, 
                                                DWORD          data = 0) PURE;

   //
   // Create a game-specific action object
   //
   STDMETHOD_(cAIAction *, CreateCustomAction)(tAIActionType type,
                                               IAIActor *  pOwner,
                                               DWORD         data = 0) PURE;

   // 
   // Factories for actions that may have proxies in network games.
   //
   STDMETHOD_(void, EnactProxyLaunchAction)     (IAI *pAI, void *netmsg) PURE;
   STDMETHOD_(void, EnactProxyJointRotateAction)(IAI *pAI, void *netmsg) PURE;
   STDMETHOD_(void, EnactProxyJointSlideAction) (IAI *pAI, void *netmsg) PURE;
   STDMETHOD_(void, EnactProxyJointScanAction)  (IAI *pAI, void *netmsg) PURE;

   STDMETHOD_(void, EnactProxyCustomAction)(tAIActionType type, IAI *pAI, void *netmsg) PURE;

   //
   // Save enough information to recreate this action.
   //
   STDMETHOD_(void, SaveAction)(ITagFile * pTagFile, IAIAction *pAction) PURE;

   //
   // Load and create an appropiate kind of action based on what is saved in pTagFile.
   //
   STDMETHOD_(cAIAction *, LoadAndCreateAction)(ITagFile *pTagFile, IAIActor *pOwner) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIBHV_H */
