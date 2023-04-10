/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/scrptnet.cpp,v 1.7 1999/06/10 20:22:34 Justin Exp $
//
// Basic Script Networking implementation
//
// scrptnet.h, in the script library, defines what the script system requires
// as far as networking is concerned. This module implements those basic
// features.
//

#include <lg.h>
#include <aggmemb.h>
#include <appagg.h>

#include <scrptapi.h>

#include <scrptnet.h>
#include <scrptne_.h>
#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>
#include <netnotif.h>

// This should come last:
#include <dbmem.h>

class cScriptNetImpl : public IScriptNet
{
   DECLARE_AGGREGATION(cScriptNetImpl);

public:
   cScriptNetImpl(IUnknown *pOuter)
      : m_pNetMan(NULL),
        m_pObjNet(NULL),
        m_bNetworking(FALSE),
        m_pPostMsg(NULL)
   {
      INIT_AGGREGATION_1(pOuter, IID_IScriptNet, this, kPriorityNormal, NULL);
   }

   virtual ~cScriptNetImpl()
   {
   }

protected:

   //////////
   //
   // Basic AppAgg methods:
   //
   STDMETHOD(Init)()
   {
      m_pPostMsg = new cNetMsg(&gm_PostMsgDesc, this);

      m_pNetMan = AppGetObj(INetManager);
      m_pObjNet = AppGetObj(IObjectNetworking);
      m_pScriptMan = AppGetObj(IScriptMan);

      // Register a listener; we need to know when the networking state
      // changes, to allow messages or not:
      m_pNetMan->Listen(netManListener, 
                        (kNetMsgNetworking | kNetMsgReset),
                        this);

      return S_OK;
   }

   STDMETHOD(End)()
   {
      SafeRelease(m_pNetMan);
      SafeRelease(m_pObjNet);
      SafeRelease(m_pScriptMan);

      if (m_pPostMsg) {
         delete m_pPostMsg;
         m_pPostMsg = NULL;
      }

      return S_OK;
   }

public:

   //////////
   //
   // The Script Networking API implementation:
   //

   //
   // Return TRUE iff the object is a proxy for one hosted elsewhere.
   //
   STDMETHOD_(BOOL, ObjIsProxy)(ObjID obj)
   {
      if (!m_bNetworking) {
         // Just let it through...
         return FALSE;
      } else {
         Assert_(m_pObjNet);
         return m_pObjNet->ObjIsProxy(obj);
      }
   }

   //
   // Tell the owner of this object to deal with the given message.
   //
   // @TBD: At the moment, we assume that any parms that are ints are in
   // fact ObjIDs, and we proxy them appropriately. In the long run, we
   // need some way to distinguish ints from ObjIDs in cMultiParm.
   //
   STDMETHOD(PostToOwner)(ObjID from,
                          ObjID to,
                          const char *message,
                          const cMultiParm &data,
                          const cMultiParm &data2,
                          const cMultiParm &data3)
   {
      AssertMsg(m_pObjNet->ObjIsProxy(to),
                "ScriptNet::PostMessage called on local object!\n");

      // Sanity check: make sure that we don't try to include local-only
      // objects in these messages.
      if ((from != OBJ_NULL) &&
          !m_pObjNet->ObjIsProxy(from) && 
          !m_pObjNet->ObjHostedHere(from))
      {
         // It's local-only
         from = OBJ_NULL;
      }
      
      m_pPostMsg->Send(OBJ_NULL, from, to, message, &data, &data2, &data3);

      return S_OK;
   }

private:

   // Callback from NetMan, when networking is turned on or off:
   void NetManListener(eNetListenMsgs situation)
   {
      switch (situation) {
         case kNetMsgNetworking:
            m_bNetworking = TRUE;
            break;
         case kNetMsgReset:
            m_bNetworking = FALSE;
            break;
         default:
            Warning(("ScriptNetImpl: Got an unknown listener msg!\n"));
            break;
      }
   }
   static void netManListener(eNetListenMsgs situation,
                              DWORD /* data */,
                              void *pScriptNet)
   {
      ((cScriptNetImpl *) pScriptNet)->NetManListener(situation);
   }

   void HandlePostMsg(ObjID from, 
                      ObjID to, 
                      const char *message, 
                      cMultiParm &data,
                      cMultiParm &data2,
                      cMultiParm &data3)
   {
      // Construct the new script message:
      sScrMsg scrMsg(from, to, message, data, data2, data3);
      // Since the message was explicitly sent to us, allow it to work
      // even if it's to a proxy object:
      scrMsg.flags |= kSMF_MsgSendToProxy;

      // And send it along
      m_pScriptMan->SendMessage(&scrMsg);
   }

   // Dispatch a PostMsg to the system:
   static void handlePostMsg(ObjID from, 
                             ObjID to, 
                             const char *message, 
                             cMultiParm &data,
                             cMultiParm &data2,
                             cMultiParm &data3,
                             void *pScriptNet)
   {
      ((cScriptNetImpl *) pScriptNet)->HandlePostMsg(from, 
                                                     to, 
                                                     message, 
                                                     data,
                                                     data2,
                                                     data3);
   }

   // The descriptor for the networked PostMessage message:
   static sNetMsgDesc gm_PostMsgDesc;

   // The Big Players in the networking world:
   INetManager *m_pNetMan;
   IObjectNetworking *m_pObjNet;

   // And the scripting world:
   IScriptMan *m_pScriptMan;

   // Are we currently sending ordinary network messages?
   BOOL m_bNetworking;

   // The actual Post message:
   cNetMsg *m_pPostMsg;
};
sNetMsgDesc cScriptNetImpl::gm_PostMsgDesc = {
   kNMF_SendToObjOwner,
   "PostMsg",
   "Post Script Message",
   NULL,
   cScriptNetImpl::handlePostMsg,
   {{kNMPT_GlobalObjID, kNMPF_None, "From"},
    {kNMPT_ReceiverObjID, kNMPF_None, "To"},
    {kNMPT_String, kNMPF_None, "Message"},
    // @HACK: note that we *always* interpret integer params as ObjIDs.
    // This really isn't right, but should work adequately for Shock.
    // For Thief, we need to come up with a better way to understand
    // whether the MultiParm is an ObjID or an int...
    {kNMPT_GlobalObjIDMultiParm, kNMPF_None, "Data"},
    {kNMPT_GlobalObjIDMultiParm, kNMPF_None, "Data2"},
    {kNMPT_GlobalObjIDMultiParm, kNMPF_None, "Data3"},
    {kNMPT_End}}
};

IMPLEMENT_AGGREGATION_SELF_DELETE(cScriptNetImpl);

//
// Creation function for script networking
//
tResult LGAPI ScriptNetworkingCreate(void)
{
   IUnknown* outer = AppGetObj(IUnknown); 
   cScriptNetImpl* net = new cScriptNetImpl(outer);
   return (net != NULL) ? NOERROR : E_FAIL;
}
