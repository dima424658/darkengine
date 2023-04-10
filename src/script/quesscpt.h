/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/quesscpt.h,v 1.6 2000/01/05 17:00:32 BFarquha Exp $
//

#ifndef __QUESSCPT_H
#define __QUESSCPT_H

#pragma once

#include <scrptsrv.h>
#include <scrptmsg.h>
#include <objscrt.h>
#include <questapi.h>

////////////////////////////////////////
//
// Services
//

DECLARE_SCRIPT_SERVICE(Quest, 0x152)
{
   // Subscribe to quest data change msgs
   STDMETHOD_(BOOL, SubscribeMsg)(object obj, const char *pName, eQuestDataType type = kQuestDataUnknown) PURE;

   // Unsubscribe to message types
   STDMETHOD_(BOOL, UnsubscribeMsg)(object obj, const char *pName) PURE;

   // Set existing data item, creates it if necessary
   STDMETHOD(Set)(THIS_ const char *pName, int value, eQuestDataType type = kQuestDataMission) PURE;

   // Get data, returns zero if undefined
   STDMETHOD_(int,Get)(THIS_ const char *pName) PURE;

   // determine whether a quest variable exists
   STDMETHOD_(BOOL,Exists)(THIS_ const char* pName) PURE;

   // Delete quest variable
   STDMETHOD_(BOOL, Delete)(THIS_ const char *pName) PURE;
};


////////////////////////////////////////
//
// Messages
//

#define kQuestMsgVer 1

struct sQuestMsg : public sScrMsg
{
   const char *m_pName;
   int m_oldValue;
   int m_newValue;

   sQuestMsg()
   {
   }

   sQuestMsg(ObjID objID, const char *pName, int oldValue, int newValue):
      m_pName(pName),
      m_oldValue(oldValue),
      m_newValue(newValue)
   {
      // setup base scrMsg fields
      to = objID;
      free((void *)message);
      message = strdup("QuestChange");
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT
#define OnQuestChange()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(QuestChange, sQuestMsg)
#define DefaultOnQuestChange()  SCRIPT_CALL_BASE(QuestChange)
#endif

#endif // __QUESSCPT_H
