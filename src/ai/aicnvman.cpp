/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicnvman.cpp,v 1.7 1998/11/18 10:41:43 JON Exp $
//
//

#include <lg.h>
#include <dlist.h>
#include <dlisttem.h>

#include <autolink.h>
#include <linkbase.h>
#include <tagfile.h>
#include <vernum.h>

#include <aiapi.h>
#include <aiapiiai.h>
#include <aicnvapi.h>
#include <aicnvman.h>
#include <aicnvrse.h>
#include <aiprconv.h>
#include <aitagtyp.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIConversationManager
//

cAIConversationManager::cAIConversationManager()
{

}

///////////////////////////////////////

cAIConversationManager::~cAIConversationManager()
{
   Reset(); 
   m_convListenerList.DestroyAll();
}

///////////////////////////////////////

BOOL cAIConversationManager::CheckActors(ObjID conversationID) 
{
   cAIConversationDesc* pConversationDesc;
   int actorID;
   ObjID objID;

   if (!g_pAIConversationProperty->Get(conversationID, &pConversationDesc))
   {
      Warning(("Object %d is not a conversation\n", conversationID));
      return FALSE;
   }
   AutoAppIPtr(AIManager);
   for (int i=0; i<kAIMaxConvSteps; i++)
      for (int j=0; j<kAIMaxConvActions; j++)
      {
         if ((actorID = pConversationDesc->GetActor(i, j))<0)
            continue;
         if (!GetActorObj(conversationID, pConversationDesc->GetActor(i, j), &objID))
            return FALSE;
         // check alertness
         IInternalAI *pInternalAI = (IInternalAI*)(pAIManager->GetAI(objID));
         if (pInternalAI == NULL)
         {
            Warning(("Object %d has no AI\n", objID));
            return FALSE;
         }
         if (pInternalAI->GetState()->GetAlertness()>=pConversationDesc->abortLevel)
         {
            pInternalAI->Release();
            return FALSE;
         }
         pInternalAI->Release();
      }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIConversationManager::Start(ObjID conversationID)
{
   if (!CheckActors(conversationID))
   {
      Warning(("Can't start conversation %d: unfilled actor slots\n", conversationID));
      return FALSE;
   }
   // create a new conversation and add to list
   IAIConversation *pConversation = new cAIConversation(conversationID);
   cConversationNode *pConversationNode = new cConversationNode;
   pConversationNode->item = pConversation;
   m_convList.Append(pConversationNode);
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::Frame(void)
{
   cConversationNode *pConversationNode;

   pConversationNode = m_convList.GetFirst();
   while (pConversationNode != NULL)
   {
      pConversationNode->item->Frame();
      pConversationNode = pConversationNode->GetNext();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::SetActorObj(ObjID conversationID, int actorID, ObjID objID)
{
   RemoveActorObj(conversationID, actorID);
   g_pAIConvActorRelation->AddFull(conversationID, objID, &actorID);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIConversationManager::GetActorObj(ObjID conversationID, int actorID, ObjID* pObjID)
{
   cAutoLinkQuery query(g_pAIConvActorRelation, conversationID, LINKOBJ_WILDCARD);
   sLink link;
   
   while (!query->Done())
   {
      void *pData = query->Data();
      if ((pData != NULL) && (*(int*)(query->Data())-1 == actorID))
      {
         query->Link(&link);
         if (pObjID != NULL)
            *pObjID = link.dest;
         return TRUE;
      }
      query->Next();
   }
   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::RemoveActorObj(ObjID conversationID, int actorID)
{
   cAutoLinkQuery query(g_pAIConvActorRelation, conversationID, LINKOBJ_WILDCARD);
   
   while (!query->Done())
   {
      if ((*(int*)(query->Data())) == actorID)
      {
         g_pAIConvActorRelation->Remove(query->ID());
         return;
      }
      query->Next();
   }
}

///////////////////////////////////////

STDMETHODIMP_(int) cAIConversationManager::GetActorIDs(ObjID conversationID, int *pActorIDs)
{
   cAIConversationDesc* pConversationDesc;
   int actorID;
   BOOL found;
   int nActors = 0;

   if (!g_pAIConversationProperty->Get(conversationID, &pConversationDesc))
   {
      Warning(("Object %d is not a conversation\n", conversationID));
      return FALSE;
   }
   // search through the conversation description and find all actorIDs referenced
   for (int i=0; i<kAIMaxConvSteps; i++)
      for (int j=0; j<kAIMaxConvActions; j++)
      {
         if ((actorID = pConversationDesc->GetActor(i, j)) == kAICA_None)
            continue;
         found = FALSE;
         for (int k=0; (k<nActors) && !found; k++)
            found = (pActorIDs[k] == actorID);
         // there should never be more actors than whatever our enum says
         Assert_(nActors<kAICA_Num);
         if (!found && (pActorIDs != NULL))
            pActorIDs[nActors++] = actorID;
      }
   return nActors;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::ListenConversationEnd(tConversationListener listener)
{
   cConversationListenerNode *pNode = new cConversationListenerNode();
   pNode->item = listener;
   m_convListenerList.Append(pNode);
}
 
///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::NotifyConversationEnd(ObjID conversationID)
{
   cConversationNode *pConversationNode;
   BOOL found = FALSE;
   cConversationListenerNode *pListenerNode;

   pConversationNode = m_convList.GetFirst();
   while ((pConversationNode != NULL) && !found)
   {
      if (conversationID == pConversationNode->item->GetConversationID())
      {
         m_convList.Remove(pConversationNode);
         SafeRelease(pConversationNode->item);
         delete pConversationNode;
         found = TRUE;
      }
      pConversationNode = pConversationNode->GetNext();
   }
   if (!found)
      Warning(("NotifyConversationEnd: no such conversation %d\n", conversationID));
   pListenerNode = m_convListenerList.GetFirst();
   while (pListenerNode != NULL)
   {
      (pListenerNode->item)(conversationID);
      pListenerNode = pListenerNode->GetNext();
   }
}

///////////////////////////////////////////////////////////
//
// Save/Load logic
//

static TagFileTag g_AIConverseTag        = { "AICONVERSE" };
static TagVersion g_AIConverseTagVersion = { 1, 01 };

static BOOL OpenConverseTagFile(ITagFile* pTagFile)
{
   HRESULT result;
   TagVersion v = g_AIConverseTagVersion;

   result = pTagFile->OpenBlock(&g_AIConverseTag, &v);

   if (result == S_OK)
   {
      if (v.major != g_AIConverseTagVersion.major || v.minor != g_AIConverseTagVersion.minor)
      {
         pTagFile->CloseBlock();
         Warning (("File contains old AI conversation data, data not loaded.\n"));
      }
   }

   return (result == S_OK  &&
           v.major == g_AIConverseTagVersion.major &&
           v.minor == g_AIConverseTagVersion.minor);
}

///////////////////////////////////////

unsigned cAIConversationManager::NumSavableConversations()
{
   unsigned num = 0;
   BOOL save;
   cConversationNode *pConversationNode = m_convList.GetFirst();
   while (pConversationNode != NULL)
   {
      save = FALSE;
      g_pAISaveConversationProperty->Get(pConversationNode->item->GetConversationID(), &save);
      if (save)
         ++num;
      pConversationNode = pConversationNode->GetNext();
   }
   return num;
}

///////////////////////////////////////

BOOL cAIConversationManager::HasConversation(ObjID convObj)
{
   cConversationNode *pConversationNode = m_convList.GetFirst();
   while (pConversationNode != NULL)
   {
      if (pConversationNode->item->GetConversationID() == convObj)
         return TRUE;
      pConversationNode = pConversationNode->GetNext();
   }
   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::Save(ITagFile *pTagFile)
{
   if (!OpenConverseTagFile(pTagFile))
      return;

   unsigned nConversations = NumSavableConversations();
   ObjID obj;
   BOOL save;

   AITagMove(pTagFile, &nConversations);
   cConversationNode *pConversationNode = m_convList.GetFirst();

   while (pConversationNode != NULL)
   {
      save = FALSE;
      obj = pConversationNode->item->GetConversationID();
      g_pAISaveConversationProperty->Get(obj, &save);
      if (save)
         AITagMove(pTagFile, &obj);
      pConversationNode = pConversationNode->GetNext();
   }
   pTagFile->CloseBlock();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIConversationManager::Load(ITagFile *pTagFile)
{
   if (!OpenConverseTagFile(pTagFile))
      return;

   unsigned nConversations;
   ObjID convObj;

   AITagMove(pTagFile, &nConversations);
   for (int i = 0; i < nConversations; i++)
   {
      AITagMove(pTagFile, &convObj);
      // If this conversation object does not already have a conversation going,
      // start one.  (Note that the save file may include the same conversation obj
      // multiple times).
      if (!HasConversation(convObj))
         Start(convObj);
   }
   pTagFile->CloseBlock();
}

////////////////////////////////////////

STDMETHODIMP cAIConversationManager::Reset()
{
   cConversationNode *pConversationNode;

   pConversationNode = m_convList.GetFirst();
   while (pConversationNode != NULL)
   {
      SafeRelease(pConversationNode->item);
      pConversationNode = pConversationNode->GetNext();
   }
   m_convList.DestroyAll();
   return S_OK; 
}

