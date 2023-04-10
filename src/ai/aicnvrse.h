/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvrse.h,v 1.4 1998/09/29 10:35:33 TOML Exp $
//
// AI Conversation structs
//

#ifndef __AICNVRSE_H
#define __AICNVRSE_H

#include <dlist.h>

#include <aicnvapi.h>
#include <aitype.h>
#include <aipsdscr.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIConverse);

///////////////////////////////////////////////////////////////////////////////
//
// Conversations
//

enum eAIConvActor
{
   kAICA_None = -1,
   kAICA_One = 0,
   kAICA_Two = 1,
   kAICA_Three = 2,
   kAICA_Four = 3,
   kAICA_Five = 4,
   kAICA_Six = 5,
   kAICA_Num,
   kAIMS_ConvActorMax = 0xffffffff
};

enum eAIConvActionFlags
{
   kAICAct_NoBlock = 0x0001,
   kAICAct_Max = 0xffffffff
};

struct sAIConvAction
{
   eAIConvActor actor;
   int flags;
   sAIPsdScrAct act;
};

#define kAIMaxConvActions 6

typedef sAIConvAction tAIConvStep[kAIMaxConvActions];

///////////////////////////////////////
//
// Description of a conversation, attached to an object
//

#define kAIMaxConvSteps 12

class cAIConversationDesc
{
public:
   DWORD             reserved1[4];
   
   eAIAlertLevel     abortLevel;
   eAIPriority       abortPriority;
   
   DWORD             reserved2[3];

   tAIConvStep       steps[kAIMaxConvSteps];

   cAIConversationDesc();

   sAIConvAction *GetAction(int step, int action) {return &(steps[step][action]);};
   int GetActor(int step, int action) const {return steps[step][action].actor;};
};

////////////////////////////////////////
//
// A conversation in progress
//

enum eAIConverseState;

class cAIConversor
{
public:
   cAIConversor(int actorID, ObjID objID);
   ~cAIConversor();

   int m_actorID;
   ObjID m_objID;
   IAIConverse *m_pAbility;
   eAIConverseState m_state;
};

// list of active AI abilities
typedef cContainerDList<cAIConversor*, 0> cAIConversorList;
typedef cContDListNode<cAIConversor*, 0> cAIConversorNode;

enum eAIConversationFlags 
{
   kAIConversationKillMe         = 0x0001, 
   kAIConversationDying          = 0x0002, 
   kAIConversationAlertBreakOut  = 0x0004, 
   kAIConversationIntMax         = 0xffffffff
};

class cAIConversation: public cCTUnaggregated<IAIConversation, &IID_IAIConversation, kCTU_Default>
{
public:
   cAIConversation(ObjID conversationID);
   ~cAIConversation();

   STDMETHOD_(void, Frame)(void);
   STDMETHOD_(void, OnStateChange)(int actorID, eAIConverseState newState, eAIConverseState oldState);
   STDMETHOD_(void, OnAlertness)(eAIAwareLevel awareness);
   STDMETHOD_(ObjID, GetConversationID)(void) {return m_objID;}

private:
   int m_flags;
   ObjID m_objID;                               // the conversation object
   int m_step;                                  // current step in the conversation
   int m_numSteps;                              // total number of steps in conversation
   cAIConversationDesc* m_pConversationDesc;    // the conversation description
   cAIConversorList m_conversors;                 // list of all conversors
   
   void StartNextStep(void);
   void End(void);

   BOOL FindConversor(int actorID, cAIConversor** ppConversor);
   void NewConversor(int actorID, ObjID objID);
   void DestroyAllConversors(void);
   BOOL ReadyToStart(void);
   BOOL FinishedWithStep(void);
   int StartActions(int step);
   void SetPriority(eAIPriority priority);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICNVRSE_H */
