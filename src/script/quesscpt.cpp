/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/quesscpt.cpp,v 1.7 2000/01/04 18:52:44 BFarquha Exp $
//

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <quesscpt.h>
#include <questapi.h>

#include <appagg.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////
//
// Quest service implementation
//

DECLARE_SCRIPT_SERVICE_IMPL(cQuestSrv, Quest)
{
public:
   // Subscribe an object to message types
   STDMETHOD_(BOOL, SubscribeMsg)(object obj, const char *pName, eQuestDataType type)
   {
      AutoAppIPtr(QuestData);
      pQuestData->SubscribeMsg(obj, pName, type);
      return TRUE;
   }

   // Unsubscribe an object to message types
   STDMETHOD_(BOOL, UnsubscribeMsg)(object obj, const char *pName)
   {
      AutoAppIPtr(QuestData);
      pQuestData->UnsubscribeMsg(obj, pName);
      return TRUE;
   }

   STDMETHOD_(int, Get)(const char *pName)
   {
      AutoAppIPtr(QuestData);
      return(pQuestData->Get(pName));
   }

   STDMETHOD(Set)(THIS_ const char *pName, int value, eQuestDataType type)
   {
      AutoAppIPtr(QuestData);
      if (pQuestData->Exists(pName))
         pQuestData->Set(pName,value);
      else
         pQuestData->Create(pName,value,type);
      return(S_OK);
   }

   STDMETHOD_(BOOL, Exists)(const char *pName)
   {
      AutoAppIPtr(QuestData);
      return pQuestData->Exists(pName);
   }

   STDMETHOD_(BOOL, Delete)(const char *pName)
   {
      AutoAppIPtr(QuestData);
      return pQuestData->Delete(pName);
   }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cQuestSrv, Quest);

////////////////////////////////////////
//
// Quest message implementation
//

IMPLEMENT_SCRMSG_PERSISTENT(sQuestMsg)
{
   PersistenceHeader(sScrMsg, kQuestMsgVer);
   return TRUE;
}
