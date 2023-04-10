/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/netscrpt.cpp,v 1.23 2000/02/19 12:36:16 toml Exp $

#include <mprintf.h>
#include <appagg.h>

#include <playrobj.h>  // for IsAPlayer()

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>

#include <netscrpt.h>

#include <cfgdbg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//////////
//
// The network message handler
//
// This code accepts network messages and dispatches them to the appropriate
// scripts.
//

static cNetMsg *g_pBroadcastScriptMsg = NULL;
static cNetMsg *g_pProxyScriptMsg = NULL;

static void SendScriptMsg(ObjID obj, 
                          const char *pText, 
                          const cMultiParm ref data)
{
   sScrMsg msg(obj, pText);
   // Since this message was explicitly sent to us, allow it to run even
   // if it's to a proxy object:
   msg.flags |= kSMF_MsgSendToProxy;
   msg.data = data;

   // And send it along
   AutoAppIPtr(ScriptMan);
   pScriptMan->SendMessage(&msg);
}

// @TBD: These message differ only in that one is broadcast, and the other
// directed. We should enhance cNetMsg to allow both to be combined into
// one message.
//
// @HACK: Note that the Send version treats its param as an objID. This
// should go away once we have a chance to make cMultiParm grok the
// difference between an int and an object. This hack is in place because
// we specifically use SendToProxy in a place that needs to send an object
// in Shock.
static sNetMsgDesc sBroadcastScriptDesc =
{
   kNMF_Broadcast,
   "BroadScrpt",
   "Broadcast Script Message",
   NULL,
   SendScriptMsg,
   {{kNMPT_GlobalObjID, kNMPF_None, "Obj"},
    {kNMPT_String, kNMPF_None, "Message"},
    {kNMPT_MultiParm, kNMPF_None, "Data"},
    {kNMPT_End}}
};

static sNetMsgDesc sProxyScriptDesc =
{
   kNMF_None,
   "ProxyScrpt",
   "Script Message to Proxy",
   NULL,
   SendScriptMsg,
   {{kNMPT_GlobalObjID, kNMPF_None, "Obj"},
    {kNMPT_String, kNMPF_None, "Message"},
    {kNMPT_GlobalObjIDMultiParm, kNMPF_None, "Data"},
    {kNMPT_End}}
};

//////////
//
// The Script handling side
//
// This stuff creates a simple script service, which lets a script tell
// all other clients of this object to also run the script.
//

DECLARE_SCRIPT_SERVICE_IMPL(cNetworkingSrv, Networking)
{
protected:
   INetManager *m_pNetMan;
   IObjectNetworking *m_pObjNet;
   // An iterator over the players, if we're iterating:
   sPropertyObjIter playerIter;

public:

   STDMETHOD_(void,Init)()
   {
#ifdef NEW_NETWORK_ENABLED
      m_pNetMan = AppGetObj(INetManager);
      m_pObjNet = AppGetObj(IObjectNetworking);

      g_pBroadcastScriptMsg = new cNetMsg(&sBroadcastScriptDesc);
      g_pProxyScriptMsg = new cNetMsg(&sProxyScriptDesc);
#endif
   }

   STDMETHOD_(void, End)()
   {
#ifdef NEW_NETWORK_ENABLED
      SafeRelease(m_pNetMan);
      SafeRelease(m_pObjNet);

      delete g_pBroadcastScriptMsg;
      delete g_pProxyScriptMsg;
#endif
   }

   // Send this message to the specified object on all other machines.
   STDMETHOD(Broadcast)(const object ref scrObj, 
                        const char *pText,
                        BOOL sendFromProxy,
                        const cMultiParm ref data)
   {
#ifdef NEW_NETWORK_ENABLED
      ObjID obj = ScriptObjID(scrObj);
      if (!sendFromProxy && m_pObjNet->ObjIsProxy(obj)) {
         // We don't own it, so suppress the message:
         return FALSE;
      }
      if (m_pObjNet->ObjLocalOnly(obj)) {
         // Never broadcast about local-only objects
         return FALSE;
      }
      g_pBroadcastScriptMsg->Send(OBJ_NULL, obj, pText, &data);
#endif
      return TRUE;
   }

   // Send the specified message to the specified object on just the
   // specified player's machine. The object should not be local-only.
   // It may be called from any machine; that is, it can be used to
   // send messages from the proxy to itself.
   //
   // Note that this method should work, even if networking is not
   // enabled; the message will loop back to the local player.
   STDMETHOD(SendToProxy)(const object ref toPlayer, 
                          const object ref obj, 
                          const char *pText,
                          const cMultiParm ref data)
   {
      ObjID target = ScriptObjID(obj);

#ifdef NEW_NETWORK_ENABLED
      g_pProxyScriptMsg->Send(ScriptObjID(toPlayer), target, pText, &data);
#else
      // If we don't have any networking, just do a short-circuit here
      SendScriptMsg(target, pText, data);
#endif
      return TRUE;
   }

   // Take over the specified object
   STDMETHOD(TakeOver)(const object ref obj)
   {
      // Make sure we should do anything
#ifndef NEW_NETWORK_ENABLED
      return TRUE;
#endif
      if (!m_pNetMan->Networking()) {
         return TRUE;
      }

      ObjID target = ScriptObjID(obj);
      m_pObjNet->ObjTakeOver(target);

      return TRUE;
   }

   // Hand off the specified object
   STDMETHOD(GiveTo)(const object ref obj, const object ref toPlayer)
   {
      // Make sure we should do anything
#ifndef NEW_NETWORK_ENABLED
      return TRUE;
#endif
      if (!m_pNetMan->Networking()) {
         return TRUE;
      }

      ObjID target = ScriptObjID(obj);
      ObjID to = ScriptObjID(toPlayer);
      m_pObjNet->ObjGiveWithoutObjID(target, to);

      return TRUE;
   }

   STDMETHOD_(BOOL, IsPlayer)(const object ref obj)
   {
      ObjID Obj = ScriptObjID(obj);
      return IsAPlayer(Obj);
   }

   STDMETHOD_(BOOL, IsMultiplayer)()
   {
#ifndef NEW_NETWORK_ENABLED
      return FALSE;
#endif
      return m_pNetMan->IsNetworkGame();
   }

   STDMETHOD_(timer_handle, SetProxyOneShotTimer)
      (const object ref toObj,
       const char *msg,
       float time,
       const cMultiParm & data = NULL_PARM)
   {
      ObjID to = ScriptObjID(toObj);
      sScrMsg * pMsg = new sScrTimerMsg(to, msg, data);
      pMsg->flags |= kSMF_MsgSendToProxy;
      AutoAppIPtr(ScriptMan);
      timer_handle result = 
         (timer_handle) pScriptMan->SetTimedMessage(pMsg, 
                                                    (time*1000), 
                                                    kSTM_OneShot);
      pMsg->Release();
      return result;
   }

   STDMETHOD_(object, FirstPlayer)()
   {
      m_pNetMan->NetPlayerIterStart(&playerIter);
      return PlayerObject();
   }

   STDMETHOD_(object, NextPlayer)()
   {
      ObjID player = OBJ_NULL;
      if (m_pNetMan->NetPlayerIterNext(&playerIter, &player))
         return player;
      else
         return OBJ_NULL;
   }

   STDMETHOD(Suspend)()
   {
      m_pNetMan->SuspendMessaging();
      return S_OK;
   }

   STDMETHOD(Resume)()
   {
      m_pNetMan->ResumeMessaging();
      return S_OK;
   }

   STDMETHOD_(BOOL, HostedHere)(const object ref obj)
   {
      return m_pObjNet->ObjHostedHere(ScriptObjID(obj));
   }

   STDMETHOD_(BOOL, IsProxy)(const object ref obj)
   {
      return m_pObjNet->ObjIsProxy(ScriptObjID(obj));
   }

   STDMETHOD_(BOOL, LocalOnly)(const object ref obj)
   {
      return m_pObjNet->ObjLocalOnly(ScriptObjID(obj));
   }

   STDMETHOD_(BOOL, IsNetworking)()
   {
      return m_pNetMan->Networking();
   }

   STDMETHOD_(object, Owner)(const object ref objRef)
   {
      ObjID obj = ScriptObjID(objRef);
      if (m_pObjNet->ObjLocalOnly(obj))
      {
         return PlayerObject();
      } else {
         return m_pObjNet->ObjHostPlayer(obj);
      }
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cNetworkingSrv, Networking);

