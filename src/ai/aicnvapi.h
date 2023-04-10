/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvapi.h,v 1.2 1998/09/21 14:53:25 JON Exp $
//
// AI Conversation Interface
//

#ifndef __AICNVAPI_H
#define __AICNVAPI_H

#include <comtools.h>
#include <aitype.h>
#include <objtype.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIConverse);
F_DECLARE_INTERFACE(IAIConversation);
F_DECLARE_INTERFACE(IAIConversationManager);

////////////////////////////////////
//
// Converse - ability interface on AIs
//

typedef struct sAIPsdScrAct sAIPsdScrAct;

DECLARE_INTERFACE_(IAIConverse, IUnknown)
{
   STDMETHOD_(void, Start)(THIS_ IAIConversation *pConversation, int actorID) PURE;
   STDMETHOD_(void, NewActions)(THIS_ sAIPsdScrAct *pAction, unsigned nActions) PURE;
   STDMETHOD_(void, Terminate)(THIS) PURE;
   STDMETHOD_(void, OnSpeechStart)(int hSchema) PURE;
   STDMETHOD_(void, OnSpeechEnd)(int hSchema) PURE;
   STDMETHOD_(void, SetPriority)(eAIPriority priority) PURE;
   STDMETHOD_(int, GetCurrentAction)(void) PURE;
};

////////////////////////////////////
//
// Conversations - one for each active conversation
//

enum eAIConverseState;

DECLARE_INTERFACE_(IAIConversation, IUnknown)
{
   // for abilities to notify the conversation of their state
   STDMETHOD_(void, OnStateChange)(THIS_ int actorID, eAIConverseState newState, eAIConverseState oldState) PURE;
   STDMETHOD_(void, OnAlertness)(eAIAwareLevel awareness) PURE;

   STDMETHOD_(void, Frame)(THIS) PURE;

   STDMETHOD_(ObjID, GetConversationID)(THIS) PURE;
};

/////////////////////////////////////
//
// Conversation Manager
//

typedef void (*tConversationListener)(ObjID conversationID);

DECLARE_INTERFACE_(IAIConversationManager, IUnknown)
{
   // try to start conversation
   // can fail if actors not set correctly
   STDMETHOD_(BOOL, Start)(THIS_ ObjID conversationID) PURE; 
   
   // set an actor (create actor link from conv to actor obj)
   STDMETHOD_(void, SetActorObj)(THIS_ ObjID conversationID, int actorID, ObjID objID) PURE;
   // get current actor obj, if any
   STDMETHOD_(BOOL, GetActorObj)(THIS_ ObjID conversationID, int actorID, ObjID* pObjID) PURE;
   // destroy actor link
   STDMETHOD_(void, RemoveActorObj)(THIS_ ObjID conversationID, int actorID) PURE;

   // how many actors are required by the conversation?
   STDMETHOD_(int, GetNumActors)(ObjID conversationID) PURE;
   // get all the actors IDs used by the conversation
   // pActorIDs should be a pointer to an array large enough to contain them all (kAICA_Num is always safe)
   STDMETHOD_(int, GetActorIDs)(ObjID conversationID, int *pActorIDs) PURE;  

   STDMETHOD_(void, Frame)(THIS) PURE;

   // install listener on conversation end
   STDMETHOD_(void, ListenConversationEnd)(tConversationListener listener) PURE;

   STDMETHOD_(void, NotifyConversationEnd)(THIS_ ObjID conversationID) PURE;
};

/////////////////////////////////////

#pragma pack()

#endif /* !__AICNVAPI_H */
