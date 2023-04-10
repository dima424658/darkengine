/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasatr.h,v 1.1 1999/03/02 17:42:26 TOML Exp $
//
// Base implementation of an AI actor
//

#ifndef __AIBASATR_H
#define __AIBASATR_H

#include <aiapiatr.h>
#include <aibascmp.h>

#pragma once
#pragma pack(4)

class cAIAction;
class cAIActions;
class cAIMoveAction;
class cAILocoAction;
class cAIMotionAction;
class cAIMoveAction;
class cAISoundAction;
class cAIOrientAction;
class cAIFrobAction;
class cAIFollowAction;
class cAIInvestAction;
class cAIWanderAction;
class cAIPsdScrAction;
class cAIWaitAction;

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cAIActorBase
//
// Classes that only implement IAIActor may derive from "cAIActor"
//

#undef INTERFACE

template <class INTERFACE, const GUID * pIID_INTERFACE>
class cAIActorBase : public cAIComponentBase<INTERFACE, pIID_INTERFACE>
{
public: 
   STDMETHOD (QueryInterface)(REFIID id, void ** ppI);

   // Current action list save/load (pTagFile cursor should be prepositioned).
   STDMETHOD_(BOOL, SaveActions)(ITagFile * pTagFile, cAIActions * pActions);
   STDMETHOD_(BOOL, LoadActions)(ITagFile * pTagFile, cAIActions * pActions);
   
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

protected:

   //
   // Factory method helpers
   //
   cAIMoveAction *   CreateMoveAction(DWORD data = 0);
   cAILocoAction *   CreateLocoAction(DWORD data = 0);
   cAIMotionAction * CreateMotionAction(DWORD data = 0);
   cAISoundAction *  CreateSoundAction(DWORD data = 0);
   cAIOrientAction * CreateOrientAction(DWORD data = 0);
   cAIFrobAction *   CreateFrobAction(DWORD data = 0);
   cAIFollowAction * CreateFollowAction(DWORD data = 0);
   cAIInvestAction * CreateInvestAction(DWORD data = 0);
   cAIWanderAction * CreateWanderAction(DWORD data = 0);
   cAIPsdScrAction * CreatePsdScrAction(DWORD data = 0);
   cAIWaitAction *   CreateWaitAction(DWORD data = 0);

   //
   // Notification helpers
   //
   
   // Did the action come from this ability?
   BOOL IsOwn(IAIAction * pAction) const;

   // Is the action message even relevant to this ability?
   BOOL IsRelevant(IAIAction * pPrevious, IAIAction * pAction) const;
   
   // Is this the first action of a new control sequence?
   BOOL IsFirstAction(IAIAction * pPrevious, IAIAction * pAction) const;

   // Use this in your SuggestActions when you just want to continue
   HRESULT ContinueActions(const cAIActions & previous, cAIActions *pNew) const;
   
};

///////////////////////////////////////////////////////////////////////////////
//
// Base class for generic actors
//

class cAIActor : public cAIActorBase<IAIActor, &IID_IAIActor>
{
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBASATR_H */
