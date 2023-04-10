/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ainetman.cpp,v 1.15 2000/02/28 11:30:06 toml Exp $
//

#include <appagg.h>
#include <cfgdbg.h>
#include <config.h>

#include <netman.h>
#include <netprops.h>
#include <netnotif.h>
#include <iobjnet.h>
#include <phnet.h>
#include <ainet.h>
#include <playrobj.h>
#include <netmsg.h>
#include <objpos.h>
#include <physapi.h>

#include <aiman.h>

#include <ai.h>
#include <aiapibhv.h>
#include <aiapimov.h>
#include <aibasbhv.h>
#include <aimanbhv.h>
#include <aiutils.h>
#include <ghostapi.h>
#include <ghostphy.h>   // Find valid pos
#include <ghostmvr.h>   // IsRemoteGhost()
#include <aiprcore.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////
//
// Modern Network Messages
//

//////////
//
// Transfer an AI
//
// This message is sent from the old owner of an AI to the player who
// should take possession of that AI.
//

void cAIManager::handleTransferAI(ObjID id, int size,
                                  void *pTransferData,
                                  cAIManager *pAIManager)
{
   sAINetTransfer transfer = {size, pTransferData};
   pAIManager->MakeFullAI(id, &transfer);
}

static sNetMsgDesc sTransferAIDesc =
{
   kNMF_None,
   "TransferAI",
   "Transfer AI",
   "ai_trans_spew",
   cAIManager::handleTransferAI,
   {{kNMPT_ReceiverObjID, kNMPF_None, "AI Obj"},
    {kNMPT_DynBlock, kNMPF_None, "Transfer Info"},
    {kNMPT_End}}
};

///////////////////////////////////////////////////////////////////////////////
//
// Query if an AI is a proxy
//

STDMETHODIMP_(BOOL) cAIManager::IsProxy(ObjID id)
{
   cAI * pAI = AccessAI(id);
   if (pAI)
      return pAI->IsNetworkProxy();
   return FALSE;
}

///////////////////////////////////////
//
// Return the information about this AI that will be needed to transfer
// it to a new host. May return NULL.
//

struct sTransferData
{
   mxs_vector loc;
};

STDMETHODIMP cAIManager::GetAITransferInfo(ObjID id,
                                           sAINetTransfer **ppTransfer)
{
   *ppTransfer = new sAINetTransfer;
   sTransferData *pData = new sTransferData;

   pData->loc = ObjPosGet(id)->loc.vec;

   (*ppTransfer)->size = sizeof(sTransferData);
   (*ppTransfer)->pData = (void *)pData;

   return S_OK;
}

///////////////////////////////////////
//
// Turn an AI into a proxy. Returns data to transmit to new host.
// Client must delete the data.
//

STDMETHODIMP cAIManager::MakeProxyAI(ObjID id, ObjID targetPlayer)
{
   AssertMsg1(!IsProxy(id), "Cannot make AI proxy: AI %d already is a proxy", id);
   if (IsProxy(id))
      return E_FAIL;

   // Get any information that we're going to need to recreate the AI
   // on the other end:
   sAINetTransfer * pTransfer;
   GetAITransferInfo(id, &pTransfer);
   if (pTransfer == NULL)
   {
      pTransfer = new sAINetTransfer;
      pTransfer->size = 0;
      pTransfer->pData = NULL;
   }

   Assert_(!(m_flags & kDeferDestroy));
   DestroyAI(id);

   // Okay, now that the AI is gone, actually hand the object off to the
   // target player.
   AutoAppIPtr(ObjectNetworking);
   pObjectNetworking->ObjGiveWithoutObjID(id, targetPlayer);
   m_pTransferAIMsg->Send(targetPlayer, id,
                          pTransfer->size, pTransfer->pData);

   delete pTransfer->pData;
   delete pTransfer;

   sAIProp * pAIProp = AIGetProperty(g_pAIProperty, id, (sAIProp *) NULL);
   Assert_(pAIProp);

   // A relatively permanant marker that this is a proxy:
   g_pAIIsProxyProperty->Set(id, TRUE);

   CreateAI(id, pAIProp->szBehaviorSet);

   return S_OK;
}

///////////////////////////////////////
//
// Turn a proxy into a full AI given received data. AI will not
// attempt to free the data.
//

STDMETHODIMP cAIManager::MakeFullAI(ObjID id, const sAINetTransfer *pTransfer)
{
   AssertMsg1(IsProxy(id), "Cannot make full AI: AI %d is not a proxy", id);
   if (!IsProxy(id))
      return E_FAIL;

   Assert_(!(m_flags & kDeferDestroy));
   DestroyAI(id);

   // If we ever do anything with the transfer data, this is where
   // to do it.

   // Note that we now own the thing:
   g_pAIIsProxyProperty->Set(id, FALSE);

   sAIProp * pAIProp = AIGetProperty(g_pAIProperty, id, (sAIProp *) NULL);
   Assert_(pAIProp);

   CreateAI(id, pAIProp->szBehaviorSet);

   AssertMsg1(pTransfer, "No tranfer data for MakeFullAI of %s!\n", ObjWarnName(id));
   if (pTransfer)
   {
      mxs_vector *pSafeLoc = &((sTransferData *)pTransfer->pData)->loc;
      mxs_vector trueLoc;

      PhysSetFlag(id, kPMF_FancyGhost, FALSE);
      _GhostFindValidLoc(id, &ObjPosGet(id)->loc.vec, pSafeLoc, &trueLoc);

      ObjTranslate(id, &trueLoc);
   }

   return S_OK;
}

//
// Check the ownership of all AIs. This is needed due to a specific
// special case: if we do a level transition to a level that we have
// already been in, and AIs have changed hands, we don't know about
// that change until *after* postload. So we need to go recheck the
// ownership, since we may have created such AIs incorrectly.
//
// @TBD (justin): When we redo object ownership to use a more robust
// mechanism that can survive level transition properly, this should
// become redundant.
//
// @NOTE: I've basically replaced this mechanism with the use of the
// g_pAIIsProxyProperty instead; that's simply more reliable. So this
// can go away once we're confident...
//
STDMETHODIMP cAIManager::RecheckAIOwnership()
{

#ifdef OLD_DUMB_OWNERSHIP_STUFF
   AutoAppIPtr(NetManager);
   if (!pNetManager->IsNetworkGame())
      // Don't worry about it
      return S_OK;

   AutoAppIPtr(ObjectNetworking);

   Assert_(!(m_flags & kDeferDestroy));

   tAIIter iter;
   IAI *pAI;
   for (pAI = GetFirst(&iter);
        pAI != NULL;
        pAI = GetNext(&iter))
   {
      ObjID obj = pAI->GetObjID();
      SafeRelease(pAI);
      if ((IsProxy(obj) && !pObjectNetworking->ObjIsProxy(obj)) ||
          (!IsProxy(obj) && pObjectNetworking->ObjIsProxy(obj)))
      {
         // This AI is confused about its network state. Kill it and
         // rebuild it, which will make it a proxy or not as needed.
         DestroyAI(obj);
         sAIProp * pAIProp =
            AIGetProperty(g_pAIProperty, obj, (sAIProp *) NULL);
         Assert_(pAIProp);
         CreateAI(obj, pAIProp->szBehaviorSet);
      }
   }
#endif

   return S_OK;
}

///////////////////////////////////////
//
// The handler id to use in network messages that should be handled by the AI manager.
//

STDMETHODIMP_(tNetMsgHandlerID) cAIManager::NetMsgHandlerID()
{
   return m_NetMsgHandlerID;
}

///////////////////////////////////////

void cAIManager::NetInit()
{
   AutoAppIPtr(NetManager);
   m_NetMsgHandlerID = pNetManager->RegisterMessageParser(NetworkMessageCallback, "AI", 0, (void*)this);
   pNetManager->Listen(AllNetPlayersJoinedCallback, kNetMsgNetworking, NULL);

   IAIBehaviorSet * pBehaviorSet = new cAINetProxyBehaviorSet;
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   // Set up modern network messages
   m_pTransferAIMsg = new cNetMsg(&sTransferAIDesc, this);
}

void cAIManager::NetTerm()
{
   delete m_pTransferAIMsg;
}

///////////////////////////////////////

// @TBD (justin): All of these messages should eventually be replaced by
// modern network messages. It's not an immediate issue, but cNetMsgs get
// a lot of automatic assertion checking, as well as all kinds of underlying
// mechanisms consistent with all other networking.

void cAIManager::NetworkMessageHandler(const sAINetMsg_Header *netmsg, ObjID from)
{
   AutoAppIPtr_(ObjectNetworking, pObjNet);

   if (from == OBJ_NULL)
   {
      // We don't allow messages from anonymous senders, since we can't
      // properly deproxify them:
      ConfigSpew("net_ai_spew",
                 ("AI received network message from unknown sender.\n"));
      return;
   }

   IAI *pIAI = GetAI(pObjNet->ObjGetProxy(from, netmsg->aiObj));
   IInternalAI *pAI = (IInternalAI *) pIAI;

   if (pAI == NULL)
   {
      Warning(("AI received network message for non-AI object\n"));
      return;
   }
   if (!(pAI->IsNetworkProxy() || pAI->GetState()->IsBrainsOnly()))
   {
      Warning(("Network message to control AI: %d which is controlled here.\n", netmsg->aiObj));
      SafeRelease(pIAI);
      return;
   }

   if (netmsg->stopping)
   {
      // Stop any action of this type.
      // Note that Move & Motion don't get stopped, they just get a new one.
      // Other actions can't just be replaced, since it must be possible for them
      // to stop and not be replaced by something else.
      ((cAI *)pAI)->StopActionOfType(netmsg->action);
   }
   else
   {
      IAIMoveEnactor *pMoveEnactor = pAI->AccessMoveEnactor();
      switch (netmsg->action)
      {
         case kAIAT_Launch:
            pAI->AccessBehaviorSet()->EnactProxyLaunchAction(pAI, (void *)netmsg);
            break;
         case kAIAT_JointRotate:
            pAI->AccessBehaviorSet()->EnactProxyJointRotateAction(pAI, (void *)netmsg);
            break;
         case kAIAT_JointSlide:
            pAI->AccessBehaviorSet()->EnactProxyJointSlideAction(pAI, (void *)netmsg);
            break;
         case kAIAT_JointScan:
            pAI->AccessBehaviorSet()->EnactProxyJointScanAction(pAI, (void *)netmsg);
            break;
         default:
            pAI->AccessBehaviorSet()->EnactProxyCustomAction(netmsg->action, pAI, (void *)netmsg);
            break;
      }
   }

   SafeRelease(pIAI);
}

///////////////////////////////////////

void cAIManager::NetworkMessageCallback(const sNetMsg_Generic *pMsg, ulong size, ObjID from, void *pClientData)
{
   cAIManager *This = (cAIManager *)pClientData;
   This->NetworkMessageHandler((sAINetMsg_Header *)pMsg, from);
}

///////////////////////////////////////

// Figure out which player should host this AI.  Return true if it is us.
//
BOOL cAIManager::ShouldHostAI(ObjID objId)
{
   BOOL hostHere;
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
#ifndef SHIP
   if (config_is_defined("ai_no_distribution"))
   {
      if (pObjNet->ObjLocalOnly(objId))
         hostHere = TRUE;
      else
         hostHere = pNetMan->AmDefaultHost(); // all are hosted by the default host.
   } else
#endif
   if (g_pAIIsProxyProperty->IsRelevant(objId))
   {
      // This AI has been handed-off to or from us
      BOOL isProxy;
      g_pAIIsProxyProperty->Get(objId, &isProxy);
      hostHere = !isProxy;
   }
   else
      // Allow for local-only AIs:
      hostHere = !(pObjNet->ObjIsProxy(objId));

   if (hostHere)
   {
      ConfigSpew("net_ai_spew", ("AI %d hosted here\n",objId));
   }

   return hostHere;
}

///////////////////////////////////////

void cAIManager::AllNetPlayersJoined()
{
   // I think this is all now pointless, although if we decide to do
   // proactive load balancing, it *might* go here...
   return;
}

///////////////////////////////////////

// static
void cAIManager::AllNetPlayersJoinedCallback(eNetListenMsgs /* situation */,
                                             DWORD /* data */,
                                             void * /* pClientData */)
{
   g_pAIManager->AllNetPlayersJoined();
}


///////////////////////////////////////

const char * cAIManager::NetSelectBehaviorSet(ObjID objId, const char * pszBehaviorSet)
{
   // Use the proxy behavior set if this is a networked game and we aren't the host.
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr_(NetManager, pNetMan);
   if (pNetMan->IsNetworkGame())
      if (!ShouldHostAI(objId))
         pszBehaviorSet = m_pBehaviorSets->Get("CustomNetProxy") ? "CustomNetProxy" : "NetProxy";
#endif
   return pszBehaviorSet;
}

#ifndef SHIP
#define _ShouldSetupGhosts() (!config_is_defined("disable_ai_ghosts"))
#else
#define _ShouldSetupGhosts() TRUE
#endif

void cAIManager::NetSetupAIGhost(ObjID objId)
{
#ifdef NEW_NETWORK_ENABLED
   if (AIShouldNotHaveGhost(objId))
      // It's some non-ghost kinda AI
      return;

   AutoAppIPtr_(NetManager, pNetMan);
   if (pNetMan->IsNetworkGame())
      if (_ShouldSetupGhosts())
         if (!IsProxy(objId))
            GhostAddLocal(objId,1.0,kGhostCfAI|kGhostCfObjPos);  // at least for now...
         else if (!IsRemoteGhost(objId))
            GhostAddRemote(objId,1.0,kGhostCfAI|kGhostCfObjPos);
#endif
}

void cAIManager::NetRemoveAIGhost(ObjID objId)
{
#ifdef NEW_NETWORK_ENABLED
   if (AIShouldNotHaveGhost(objId))
      return;

   AutoAppIPtr_(NetManager, pNetMan);
   if (pNetMan->IsNetworkGame())
      if (_ShouldSetupGhosts())
         if (!IsProxy(objId))
            GhostRemLocal(objId);
         else
            GhostRemRemote(objId);
#endif
}

///////////////////////////////////////
//
// Query if in a network game
//

STDMETHODIMP_(BOOL) cAIManager::Networking()
{
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(NetManager);
   return pNetManager->Networking();
#else
   return FALSE;
#endif
}

///////////////////////////////////////
//
// Get a list of all player objects. List is null terminated; also returns number
//

STDMETHODIMP_(int) cAIManager::GetPlayers(tAIPlayerList * pList)
{
   (*pList)[0] = PlayerObject();
   int i = 1;

#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(NetManager);
   ObjID netPlayer;

   if (pNetManager->Networking())
   {
      FOR_ALL_NET_PLAYERS(pNetManager, &netPlayer)
      {
         if (i >= kAI_MaxPlayerList)
            break;
         (*pList)[i] = netPlayer;
         i++;
      }
   }
#endif
   (*pList)[i] = 0;
   return i;
}

///////////////////////////////////////
//
// Set the physics for a ballistic AI
//

STDMETHODIMP_(void) cAIManager::SetObjImpulse(ObjID obj, float x, float y, float z, float facing, BOOL rotating)
{
// @TBD (toml 05-05-99):
   ::SetObjImpulse(obj, x, y, z, facing, rotating);
}

///////////////////////////////////////
//
// Initiate a transfer of an AI to another machine
//

STDMETHODIMP_(BOOL) cAIManager::TransferAI(ObjID objAI, ObjID objDestPlayer)
{
   int i = GetAIIndex(objAI);

   if (i != kAINoIndex)
   {
      if (!(m_flags & kDeferDestroy))
      {
         AutoAppIPtr(AINetServices);
         if (IsAPlayer(objDestPlayer) &&
             !IsPlayerObj(objDestPlayer) &&
             !AIShouldNotHandoff(objAI) &&
             pAINetServices->Networking())
         {
            if (!config_is_defined("ai_no_distribution"))
            {
               AutoAppIPtr_(ObjectNetworking, pObjNet);
               if (pObjNet->ObjHostedHere(objAI))
               {
                  MakeProxyAI(objAI, objDestPlayer);
               }
            }
            // else just keep running on this machine
         }
         return FALSE;
      }
      else
      {
         m_Transfers[m_Transfers.Grow()].Set(objAI, objDestPlayer);
      }
      return TRUE;
   }

   return FALSE;
}


///////////////////////////////////////
//
// Initiate a transfer of an AI to another machine
//

void cAIManager::TransferDeferred()
{
   Assert_(!(m_flags & kDeferDestroy));

   for (int i = 0; i < m_Transfers.Size(); i++)
      TransferAI(m_Transfers[i].ai, m_Transfers[i].destination);

   m_Transfers.SetSize(0);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIManager::GetTargetVel(ObjID ai, sAIImpulse * pResult)
{
   cAI *            pAI = AccessAI(ai);
   IAIMoveEnactor * pMoveEnactor;
   if (pAI)
   {
      pMoveEnactor = pAI->AccessMoveEnactor();
      if (pMoveEnactor)
         return pMoveEnactor->GetTargetVel(pResult);
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
