/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvabl.h,v 1.2 1998/11/03 22:56:59 MROWLEY Exp $
//
//

#ifndef __AICNVABL_H
#define __AICNVABL_H

#include <dynarray.h>

#include <aicnvapi.h>

#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitConverseAbility(IAIManager *);
BOOL AITermConverseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIConverse
//

// state of a conversor
enum eAIConverseState {kAIConvNotStarted, kAIConvStarting, kAIConvRequestedGoal, kAIConvStarted, 
   kAIConvInProgress, kAIConvWaitingForSpeech, kAIConvFinishedAction, kAIConvFinished};

class cAIConverse : public cAIAbility, public IAIConverse
{
public:
   cAIConverse();

   STDMETHOD(QueryInterface)(REFIID id, void** ppI);
   STDMETHOD_(ULONG, AddRef)(void);
   STDMETHOD_(ULONG, Release)(void);

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   STDMETHOD_(void, Term)();

   // Override action list save/load
   STDMETHOD_(BOOL, SaveActions)(ITagFile * pTagFile, cAIActions * pActions);
   STDMETHOD_(BOOL, LoadActions)(ITagFile * pTagFile, cAIActions * pActions);
   
   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionChange)(IAIAction * pPrevious, IAIAction * pAction);

   // Conversation manager communication
   STDMETHOD_(void, Start)(IAIConversation *pConversation, int actorID);
   STDMETHOD_(void, NewActions)(sAIPsdScrAct *pAction, unsigned nActions);
   STDMETHOD_(void, Terminate)(void);
   STDMETHOD_(void, SetPriority)(eAIPriority priority);
   STDMETHOD_(int, GetCurrentAction)(void);

   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   STDMETHOD_(void, OnSpeechStart)(int hSchema);
   STDMETHOD_(void, OnSpeechEnd)(int hSchema);
   static void SpeechEndCallback(ObjID speakerID, int hSchema, ObjID schemaID);

private:
   eAIConverseState m_state;
   IAIConversation *m_pConversation;
   sAIPsdScrAct* m_pActionScript;      // current actions to do
   unsigned m_nActions;                // number of actions
   int m_actorID;                      // what actor I am 
   IAIAction* m_pCurrentAction;
   int m_hSchema;                      // if speaking, current schema handle
   eAIPriority m_priority;

   void SetState(eAIConverseState state);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICNVABL_H */
