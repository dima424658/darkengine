/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptmsg.h,v 1.15 1999/01/20 18:45:45 JUSTIN Exp $
//
//
//

#ifndef __SCRPTMSG_H
#define __SCRPTMSG_H

#pragma once

#ifdef __cplusplus

#include <string.h>

#include <comtools.h>
#include <dynarray.h>
#include <str.h>

#include <scrpttyp.h>
#include <persist.h>


///////////////////////////////////////////////////////////////////////////////
//
// Base script message
//
// The easiest way to create your own message is to simply
// derive, provide an initializer macro, and a handler signature
// macro

///////////////////////////////////////

#define kScrMsgNameMax        31
#define kScrMsgReplyStringMax 31

// User-settable flags:
// If the message has MsgSendToProxy set, then the message will be sent
// even if the target object is a local proxy for a remote object.
// If the message has MsgPostToOwner set, then the message must be posted
// (not sent); it will be posted to the owner's machine, if that is
// not this one.

enum eScrMsgFlags
{
   kSMF_MsgSent  = 0x01,
   kSMF_MsgBlock = 0x02,
   kSMF_MsgSendToProxy = 0x04,
   kSMF_MsgPostToOwner = 0x08
};

#define kScrMsgVer 1

///////////////////////////////////////

#ifndef SCRIPT
   #define DECLARE_SCRMSG() PERSIST_DECLARE()
   #define DECLARE_SCRMSG_PERSISTENT() PERSIST_DECLARE_PERSISTENT()
#else
   #define DECLARE_SCRMSG()
   #define DECLARE_SCRMSG_PERSISTENT()
#endif

#define IMPLEMENT_SCRMSG(MSG) PERSIST_IMPLEMENT(MSG)
#define IMPLEMENT_SCRMSG_PERSISTENT(MSG) PERSIST_IMPLEMENT_PERSISTENT(MSG)

///////////////////////////////////////

struct sScrMsg : public cCTUnaggregated<IUnknown, &IID_IUnknown, kCTU_Default>,
                 public sPersistent
{
   ////////////////////////////////////
   //
   // Data
   //

   // Source, if any
   ObjID         from;
   
   // Recipient
   ObjID         to;

   // Message
   const char *  message;

   // Message time
   ulong         time;

   // Flags
   int           flags;
   
   // Generic data fields
   cMultiParm    data;
   cMultiParm    data2;
   cMultiParm    data3;

   ////////////////////////////////////
   //
   // Functions
   //

   sScrMsg();
   sScrMsg(ObjID from, ObjID to, const char * pszMessage, 
           const cMultiParm & initData);
   sScrMsg(ObjID from, ObjID to, const char * pszMessage, 
           const cMultiParm & initData, const cMultiParm & initData2, 
           const cMultiParm & initData3);
   sScrMsg(ObjID toWhom, const char * pszMessage);
   virtual ~sScrMsg()
   {
      if (message)
         free((void *)message);
   }
   
   void Set(ObjID toWhom, const char * pszMessage);

#ifndef SCRIPT
   DECLARE_SCRMSG_PERSISTENT();

   int  PersistentVersion(int version);

#define PersistenceHeader(baseclass, version) \
   if (!baseclass::Persistence() || \
       PersistentVersion(version) != (version)) \
      return FALSE

#endif // ~SCRIPT
};


///////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Base messages
//

///////////////////////////////////////
//
// MESSAGE: "Timer" (simple timer)
//

#define kScrTimerMsgVer 1

struct sScrTimerMsg : public sScrMsg
{
   string     name;

   sScrTimerMsg()
   {
   }

   sScrTimerMsg(ObjID to, const char * pszName, const cMultiParm & initData)
    : sScrMsg(to, "Timer"),
      name(pszName)
   {
      data = initData;
   }

   DECLARE_SCRMSG_PERSISTENT();
};

///////////////////////////////////////////////////////////////////////////////
//
// sScrMsg, inline functions
//

inline sScrMsg::sScrMsg()
 : from(0),
   to(0),
   message(NULL),
   time(0),
   flags(0)
{
}

///////////////////////////////////////

inline sScrMsg::sScrMsg(ObjID fromWhom, ObjID toWhom, const char * pszMessage, const cMultiParm & initData)
 : from(fromWhom),
   to(toWhom),
   message(strdup(pszMessage)),
   time(0),
   flags(0),
   data(initData)
{
   
}

///////////////////////////////////////

inline sScrMsg::sScrMsg(ObjID fromWhom, ObjID toWhom, const char * pszMessage, 
                        const cMultiParm & initData, const cMultiParm & initData2, const cMultiParm & initData3)
 : from(fromWhom),
   to(toWhom),
   message(strdup(pszMessage)),
   time(0),
   flags(0),
   data(initData),
   data2(initData2),
   data3(initData3)
{
   
}

///////////////////////////////////////

inline sScrMsg::sScrMsg(ObjID toWhom, const char * pszMessage)
 : from(0),
   to(toWhom),
   message(strdup(pszMessage)),
   time(0),
   flags(0)
{
}

inline void sScrMsg::Set(ObjID toWhom, const char * pszMessage)
{
   if (message)
      free((void *)message);
   to = toWhom;
   message = pszMessage;
   time = 0;
   flags = 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// Macros for defining message handlers in script files
//

#ifdef SCRIPT

#define MESSAGE_DEFS public

#define SCRIPT_MESSAGE_HANDLER_SIGNATURE_(msg, msg_struct) \
   virtual void On##msg##_(msg_struct & message, cMultiParm * pReply)

#define SCRIPT_MESSAGE_HANDLER_SIGNATURE(msg) \
   virtual void On##msg##_(sScrMsg & message, cMultiParm * pReply)

#define SCRIPT_DEFINE_MESSAGE_(msg, msg_struct) \
   void Route##msg(sScrMsg * pMsg, cMultiParm * pReply) \
   { \
      On##msg##_(*((msg_struct *)(pMsg)), (pReply)); \
   } \
   \
   SCRIPT_MESSAGE_HANDLER_SIGNATURE_(msg, msg_struct)

#define SCRIPT_DEFINE_MESSAGE_DEFAULT_(msg, msg_struct) \
   SCRIPT_DEFINE_MESSAGE_(msg, msg_struct) \
   { \
   }

#define SCRIPT_DEFINE_MESSAGE(msg) \
   SCRIPT_DEFINE_MESSAGE_(msg, sScrMsg)

#define SCRIPT_DEFINE_MESSAGE_DEFAULT(msg) \
   SCRIPT_DEFINE_MESSAGE_DEFAULT_(msg, sScrMsg)

#define SCRIPT_CALL_BASE(msg) \
   Base::On##msg##_(message, pReply)

#define SCRIPT_BEGIN_MESSAGE_MAP() \
   virtual void DoAddMsgMaps() \
   { \
      Base::DoAddMsgMaps();

#define SCRIPT_MSG_MAP_ENTRY(msg) \
      AddMsgMapEntry(#msg, (tRouterFunc)Route##msg);

#define SCRIPT_END_MESSAGE_MAP()   \
   }

///////////////////////////////////////

#else // SCRIPT

inline int sScrMsg::PersistentVersion(int version)
{
   (*gm_pfnIO)(gm_pContextIO, &version, sizeof(int));

   return version;
}

#endif // SCRIPT


///////////////////////////////////////////////////////////////////////////////
#else /* !__cplusplus */
typedef struct sScrMsg sScrMsg;
#endif
#endif /* !__SCRPTMSG_H */
