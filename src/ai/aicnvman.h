/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvman.h,v 1.4 1998/11/09 02:37:58 mahk Exp $
//
// AI Conversation Manager
//

#ifndef __AICNVMAN_H
#define __AICNVMAN_H

#include <comtools.h>
#include <dlist.h>

#include <aicnvapi.h>

#include <objtype.h>

enum eAIConvActor;

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
// The conversation manager
//
typedef cContainerDList<IAIConversation*, 0> cConversationList;
typedef cContDListNode<IAIConversation*, 0> cConversationNode;

typedef cContainerDList<tConversationListener, 0> cConversationListenerList;
typedef cContDListNode<tConversationListener, 0> cConversationListenerNode;

class cAIConversationManager: public cCTUnaggregated<IAIConversationManager, &IID_IAIConversationManager, kCTU_Default>
{
public:
   cAIConversationManager();
   ~cAIConversationManager();

   STDMETHOD_(BOOL, Start)(ObjID conversationID);
   
   STDMETHOD_(void, SetActorObj)(ObjID conversationID, int actorID, ObjID objID);
   STDMETHOD_(BOOL, GetActorObj)(ObjID conversationID, int actorID, ObjID* pObjID);
   STDMETHOD_(void, RemoveActorObj)(ObjID conversationID, int actorID);
   STDMETHOD_(int, GetNumActors)(ObjID conversationID) {return GetActorIDs(conversationID, NULL);}
   STDMETHOD_(int, GetActorIDs)(ObjID conversationID, int *pActorIDs);  

   STDMETHOD_(void, Frame)(void);

   // install conversation end listener
   STDMETHOD_(void, ListenConversationEnd)(tConversationListener listener);

   // callback from conversations
   STDMETHOD_(void, NotifyConversationEnd)(ObjID conversationID);

   // Save/load all ongoing conversations.
   STDMETHOD_(void, Save)(ITagFile *pTagFile);
   STDMETHOD_(void, Load)(ITagFile *pTagFile);

   // Reset the database 
   STDMETHOD(Reset)(); 

protected:
   unsigned NumSavableConversations();
   BOOL HasConversation(ObjID convObj);

private:
   BOOL CheckActors(ObjID conversationID);
   cConversationList m_convList; 
   cConversationListenerList m_convListenerList;
};

#pragma pack()

#endif /* !__AICNVMAN_H */



