/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netmsg.cpp,v 1.34 1999/11/02 16:32:12 Justin Exp $
//
// Implementation of basic network messages.
//
// @TBD (justin 3/28/99): Restructure this a bit, to have a clearer concept
// of "type". Pull all of the code regarding a type together into a class,
// with inline methods implementing how that type is marshalled, unmarshalled,
// spewed, sized, and so on. Then we can get rid of those silly giant
// switch statements, and replace them with something more elegant, and
// make it more straightforward to add new types. Some of the methods can
// probably have sensible defaults, based on the size of the type, but note
// that there are several types that are transmitted in truncated form.
//

#include <mprintf.h>
#include <stdarg.h>
#include <appagg.h>
#include <cfgdbg.h>
#include <matrix.h>
#include <multparm.h>
#include <traitman.h>

#include <iobjsys.h>
#include <iobjnet.h>
#include <netman.h>
#include <netprops.h>

#include <playrobj.h>

#include <netmsg.h>
#include <netnotif.h>

#include <config.h>
#include <objedit.h>   // For ObjEditName()

// Must be last:
#include <dbmem.h>

//////////
//
// STATIC STUFF

// Constants:
// Token that indicates a *null* string, as opposed to a merely empty one:
#define NULL_STR_TOKEN 255
// The maximum allowed length of a message:
#define MAX_MESSAGE_SIZE (8 * 1024)
// The largest number of parameters than we can currently handle on
// the output side. This should be one less than the largest tMsgFunc,
// defined at SendToHandler. Make sure that ForwardTo also stays in
// synch with this.
#define MAX_DISPATCH 8
// Do the Draconian Archetype Checks. When enabled, this transmits the
// archetype of each object along with the object itself, and checks it
// on the other side. This is a tad expensive in bandwidth, but should
// allow us to test for object consistency.
#ifdef PLAYTEST
#define CHECK_ARCHETYPES 1
#endif

// Initializers for the static members:
int cNetMsg::gm_NumMsgs = 0;
BOOL cNetMsg::gm_bNetworking = FALSE;
INetManager *cNetMsg::gm_pNetMan = NULL;
IObjectNetworking *cNetMsg::gm_pObjNet = NULL;
IObjectSystem *cNetMsg::gm_pObjSys = NULL;
uchar *cNetMsg::gm_pMsgBuffer = NULL;
BOOL cNetMsg::gm_bAssertArchetypes = NULL;

// Static constructor, for the whole system
void cNetMsg::InitNetMsgs()
{
   gm_pNetMan = AppGetObj(INetManager);
   gm_pObjNet = AppGetObj(IObjectNetworking);
   gm_pObjSys = AppGetObj(IObjectSystem);
   // Initialize the networking flag.
   gm_bNetworking = gm_pNetMan->Networking();
   // Set up a listener, so we can keep track of whether we're networking
   // ourselves:
   gm_pNetMan->Listen(NetListen,
                      kNetMsgNetworking | kNetMsgReset | kNetMsgNetworkLost,
                      NULL);
   // Allocate our message buffer:
   gm_pMsgBuffer = (uchar *) malloc(MAX_MESSAGE_SIZE);
#ifdef CHECK_ARCHETYPES
   gm_bAssertArchetypes = config_is_defined("net_assert_archetypes");
#endif
}

// Static destructor, for the whole system
void cNetMsg::TermNetMsgs()
{
   SafeRelease(gm_pNetMan);
   SafeRelease(gm_pObjNet);
   SafeRelease(gm_pObjSys);
   free(gm_pMsgBuffer);
}

// Listener. This will be called by Netman whenever something really
// interesting happens:
void cNetMsg::NetListen(eNetListenMsgs situation,
                        DWORD /* data */,
                        void * /*pClientData */)
{
   switch(situation)
   {
      case kNetMsgNetworking:
         gm_bNetworking = TRUE;
         break;
      case kNetMsgNetworkLost:
      case kNetMsgReset:
         gm_bNetworking = FALSE;
         break;
   }
}

// Top-level dispatcher. This will send network messages to the right
// cNetMsg object.
void cNetMsg::NetDispatcher(const sNetMsg_Generic *pMsg,
                            ulong msgSize,
                            ObjID fromPlayer,
                            void *pClientData)
{
   cNetMsg *pHandler = (cNetMsg *) pClientData;
   pHandler->HandleMsg(pMsg, msgSize, fromPlayer);
}

//////////
//
// STARTUP / SHUTDOWN
//

// Constructor
cNetMsg::cNetMsg(sNetMsgDesc *pDesc, void *pClientData)
   : m_pDesc(pDesc),
     m_pClientData(pClientData),
     m_bForwarding(FALSE)
{
#ifndef NEW_NETWORK_ENABLED
   return;
#endif

   if (!gm_NumMsgs) {
      // This is the first message registered, so initialize the whole
      // system:
      InitNetMsgs();
   }
   gm_NumMsgs++;

   // Make sure we have a spew name
   if (!(m_pDesc->spewName)) {
      m_pDesc->spewName = "net_spew";
   }

#ifdef PLAYTEST
   m_bSpewMe = FALSE;
#ifdef SPEW_ON
   // Does the config specifically say to spew this message?
   // We spew if config_spew is on in general, and if *either* this
   // message's spewName was given in the config files *or* there is
   // a line saying:
   //   net_msg_spew <shortName>
   // The spewName version is intended to allow us to separate out
   // major different message types, while leaving most under the
   // usual net_spew; the shortName version is intended to allow us
   // to focus on a single message on an ad hoc basis. Note that the
   // shortName version works in OPT builds, which spewName version
   // doesn't.
   //
   // Yet another option is to turn on net_opt_spew, which says to
   // do the designated spewage even in an opt build. So net_spew
   // plus net_opt_spew will spew 'most everything, even in opt.
   //
   // Note that this code *must* stay in synch with cfgdbg.h.
   if (config_spew_on && CfgSpewTest((char *) m_pDesc->spewName))
   {
      m_bSpewMe = TRUE;
   }
#endif
   char msgSpewName[64];
   if (config_is_defined("net_opt_spew")
       && CfgSpewTest((char *) m_pDesc->spewName))
   {
      m_bSpewMe = TRUE;
   }
   if (config_get_raw("net_msg_spew", msgSpewName, sizeof(msgSpewName)) &&
       !strcmp(msgSpewName, m_pDesc->shortName))
   {
      m_bSpewMe = TRUE;
   }
#endif

   // How many params do we have?
   m_NumParams = 0;
   m_NumDispatchParams = 0;
   sNetMsgParam *pParam = m_pDesc->params;
   while (pParam->type != kNMPT_End) {
      m_NumDispatchParams++;
      if (pParam->type == kNMPT_DynBlock) {
         // DynBlocks dispatch *two* params, a size and a pointer:
         m_NumDispatchParams++;
      }
      m_NumParams++;
      pParam++;
   }

   // Put together the receipt param array:
   if (m_pDesc->behaviour & kNMF_AppendSenderID) {
      // We want to have a pseudo-param at the end for the player ID:
      m_NumDispatchParams++;
   }

   // If this Assertion gets thrown, it means that we need to increase
   // MAX_DISPATCH, add more tMsgFuncs, and enhance SendToHandler() to
   // deal with it. If it appears to getting thrown spuriously, you've
   // probably forgotten to put a kNMPT_End at the end of your
   // parameter list:
   AssertMsg(m_NumDispatchParams <= MAX_DISPATCH,
             "cNetMsg: too many params! Enhance me! (Or missing kNMPT_End)");

   m_ppParams = (DWORD *) malloc(m_NumDispatchParams * sizeof(DWORD));

   // Tell the net manager about ourselves:
   ulong flags = 0;
   if (m_pDesc->behaviour & kNMF_AlwaysBroadcast)
      flags |= kNetAllowNonNetwork;
   m_handlerID = gm_pNetMan->RegisterMessageParser(NetDispatcher,
                                                   m_pDesc->shortName,
                                                   flags,
                                                   this);
}

// Destructor
cNetMsg::~cNetMsg()
{
#ifndef NEW_NETWORK_ENABLED
   return;
#endif

   ConfigSpew("net_spew", ("Freeing netmsg %s (%d).\n", 
                           m_pDesc->shortName,
                           m_handlerID));

   free(m_ppParams);

   gm_NumMsgs--;
   if (!gm_NumMsgs) {
      // That's all she wrote; all messages have gone away, so shut down:
      TermNetMsgs();
   }
}

//////////
//
// MESSAGE SENDING
//

//
// Send a message out over the network. Note that this is varargs, so
// there can't be any compile-time type checking. Caveat sender.
//
// @TBD: come up with a more efficient mechanism for dispatching
// SendToObjOwner messages that are simply looping back to this machine.
// Currently, this goes all the way through to the low-level loopback in
// Netman, which means that we have to fully marshal and unmarshal the
// arguments. We should probably put in some special-case code to cut out
// most of that, which will save considerable time for many local messages.
//
void cNetMsg::Send(ObjID toPlayer, ...)
{
#ifndef NEW_NETWORK_ENABLED
   return;
#endif

   if (!gm_bNetworking) {
      if (m_pDesc->behaviour & kNMF_MetagameBroadcast) {
         // Only send metagame broadcasts if we're at least in a networked
         // game:
         if (!gm_pNetMan->IsNetworkGame()) {
            return;
         }
      } else if (m_pDesc->behaviour & kNMF_Broadcast) {
         // No broadcasts go out until networking is turned on:
         return;
      } else {
         // This is a send. Continue along; we may be doing loopback,
         // so we have to check further down.
      }
   }

   // If we're suspended, don't bother assembling broadcasts. But do
   // let through Sends (which might be loopbacks), and MetagameBroadcasts
   // (which can always go through).
   if (gm_pNetMan->Suspended() &&
       (m_pDesc->behaviour & kNMF_Broadcast))
   {
      return;
   }

   va_list pArgs;
   BOOL isLoopback = FALSE;

   // Check for loopbacks
   if ((m_pDesc->behaviour & kNMF_SendToObjOwner) &&
       (m_pDesc->params[0].type == kNMPT_ReceiverObjID))
   {
      // This is being sent to an object's owner; is that me?
      // Note that we only check the first param for loopback; therefore,
      // for efficiency's sake, it's good to put your ReceiverObjID first.
      va_start(pArgs, toPlayer);
      ObjID toObj = va_arg(pArgs, ObjID);
      if (!gm_pObjNet->ObjIsProxy(toObj))
         isLoopback = TRUE;
      va_end(pArgs);
   }
   if ((m_pDesc->behaviour & kNMF_SendToHost) && gm_pNetMan->AmDefaultHost())
      isLoopback = TRUE;
   if (isLoopback) {
      va_start(pArgs, toPlayer);
      DoLoopback(pArgs);
      va_end(pArgs);
      return;
   }

   // Marshal the arguments into the buffer:
   va_start(pArgs, toPlayer);
   ObjID objPlayer;
   uint msgSize;
   BOOL keepGoing = MarshalArguments(pArgs, &objPlayer, &msgSize);
   va_end(pArgs);

   // Did we discover that we don't want to send this?
   if (!keepGoing) {
      return;
   }

   // Spew, if desired.
#ifdef PLAYTEST
   if (m_bSpewMe)
   {
      va_start(pArgs, toPlayer);
      SendSpew(pArgs);
      va_end(pArgs);
   }
#endif

   // Send it along:
   if (m_pDesc->behaviour & kNMF_MetagameBroadcast) {
      // Broadcast it to all players during metagame
      BOOL always = (m_pDesc->behaviour & kNMF_AlwaysBroadcast);
      gm_pNetMan->MetagameBroadcast(gm_pMsgBuffer, msgSize, always);
   } else if (m_pDesc->behaviour & kNMF_Broadcast) {
      // Normal broadcast message
      gm_pNetMan->Broadcast(gm_pMsgBuffer,
                            msgSize,
                            !(m_pDesc->behaviour & kNMF_Nonguaranteed));
   } else if (m_pDesc->behaviour & kNMF_SendToObjOwner) {
      // Send to the owner of the receive-object param
      if ((objPlayer == OBJ_NULL) && (PlayerObject() != OBJ_NULL)) {
         // Badness -- no appropriate arguments
         Warning(("NetMsg: don't know which player to send to!\n"));
      } else {
         gm_pNetMan->Send(objPlayer,
                          gm_pMsgBuffer,
                          msgSize,
                          !(m_pDesc->behaviour & kNMF_Nonguaranteed));
      }
   } else if (m_pDesc->behaviour & kNMF_SendToHost) {
      // Send to the default host
      gm_pNetMan->Send((gm_bNetworking ?
                        gm_pNetMan->DefaultHost() :
                        PlayerObject()),
                       gm_pMsgBuffer,
                       msgSize,
                       !(m_pDesc->behaviour & kNMF_Nonguaranteed));
   } else {
      // Send to whoever they specified
      if ((toPlayer == OBJ_NULL) && (PlayerObject() != OBJ_NULL)) {
         // Badness -- no appropriate arguments
         Warning(("NetMsg: don't know which player to send to!\n"));
      } else {
         // If networking isn't running, we can only send to ourself:
         AssertMsg((gm_bNetworking 
                    ? TRUE
                    : (toPlayer == PlayerObject())),
                   "Can't send to other players before networking ready!");
         gm_pNetMan->Send(toPlayer,
                          gm_pMsgBuffer,
                          msgSize,
                          !(m_pDesc->behaviour & kNMF_Nonguaranteed));
      }
   }
}

// Put together the message itself
BOOL cNetMsg::MarshalArguments(va_list pArgs, ObjID *objPlayer, uint *size)
{
   sNetMsgParam *pParam = &(m_pDesc->params[0]);
   uchar *ptr = gm_pMsgBuffer;

   // Put in the message ID:
   *((tNetMsgHandlerID *) ptr) = m_handlerID;
   ptr += sizeof(tNetMsgHandlerID);

   while (pParam->type != kNMPT_End) {
      switch(pParam->type) {
         case kNMPT_Int:
         case kNMPT_UInt:
            *((uint *) ptr) = va_arg(pArgs, uint);
            ptr += sizeof(uint);
            break;

         case kNMPT_Short:
         case kNMPT_UShort:
            *((ushort *) ptr) = va_arg(pArgs, ushort);
            ptr += sizeof(ushort);
            break;

         case kNMPT_Byte:
         case kNMPT_UByte:
            *((uchar *) ptr) = va_arg(pArgs, uchar);
            ptr += sizeof(uchar);
            break;

         case kNMPT_BOOL:
            *((uchar *) ptr) = va_arg(pArgs, BOOL);
            ptr += sizeof(uchar);
            break;

         case kNMPT_SenderObjID:
         {
            ObjID obj = va_arg(pArgs, ObjID);
            // Make sure that this object really is ours, if asked:
            if (gm_bNetworking) {
               if (pParam->flags & kNMPF_IfHostedHere) {
                  // We should only keep going if the object is really
                  // owned by this player:
                  if (!gm_pObjNet->ObjHostedHere(obj)) {
                     return FALSE;
                  }
               } else {
                  // Yes, this is messy. It says that the object is
                  // okay if we're not checking, or if it's OBJ_NULL and
                  // we're explicitly allowing that, or if it's hosted here.
                  // Or if it's a message we're just forwarding on.
                  AssertMsg3(m_bForwarding ||
                             (pParam->flags & kNMPF_NoAssertHostedHere) ||
                             ((pParam->flags & kNMPF_AllowObjNull) &&
                              (obj == OBJ_NULL)) ||
                             gm_pObjNet->ObjHostedHere(obj),
                             "%s: trying to send %s object %s",
                             m_pDesc->msgName,
                             gm_pObjNet->ObjLocalOnly(obj) ?
                               "Local" : "Unowned",
                             ObjEditName(obj));
               }
            }
            // We have to use GIDs, to make messages forwardable:
            sGlobalObjID GID = gm_pNetMan->ToGlobalObjID(obj);
            *((sGlobalObjID *) ptr) = GID;
            ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
            AutoAppIPtr(TraitManager);
            ObjID arch = OBJ_NULL;
            if (obj != OBJ_NULL) {
               arch = pTraitManager->GetArchetype(obj);
            }
            *((NetObjID *) ptr) = arch;
            ptr += sizeof(NetObjID);
#endif
            break;
         }
            
         case kNMPT_ReceiverObjID:
         {
            // We have a proxy; translate it to the owner's lingo:
            ObjID proxy = va_arg(pArgs, ObjID);
            if (gm_bNetworking) {
               if (gm_pObjNet->ObjIsProxy(proxy) ||
                   gm_pObjNet->ObjHostedHere(proxy))
               {
                  *objPlayer = gm_pObjNet->ObjHostPlayer(proxy);
               } else {
                  // It's a local-only object, so loopback:
                  *objPlayer = PlayerObject();
               }
            } else {
               // This had better be a loopback:
               *objPlayer = PlayerObject();
            }
            // We have to use GIDs, to make messages forwardable:
            sGlobalObjID GID = gm_pNetMan->ToGlobalObjID(proxy);
            *((sGlobalObjID *) ptr) = GID;
            ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
            AutoAppIPtr(TraitManager);
            ObjID arch = OBJ_NULL;
            if (proxy != OBJ_NULL) {
               arch = pTraitManager->GetArchetype(proxy);
            }
            *((NetObjID *) ptr) = arch;
            ptr += sizeof(NetObjID);
#endif
            break;
         }

         case kNMPT_AbstractObjID:
         {
            ObjID obj = va_arg(pArgs, ObjID);
            AssertMsg1(OBJ_IS_ABSTRACT(obj),
                       "Object %d isn't abstract!", obj);
            *((NetObjID *) ptr) = obj;
            ptr += sizeof(NetObjID);
            break;
         }

         case kNMPT_ObjID:
            *((NetObjID *) ptr) = va_arg(pArgs, ObjID);
            ptr += sizeof(NetObjID);
            break;

         case kNMPT_GlobalObjID:
         {
            ObjID obj = va_arg(pArgs, ObjID);
            sGlobalObjID GID = gm_pNetMan->ToGlobalObjID(obj);
            *((sGlobalObjID *) ptr) = GID;
            ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
            AutoAppIPtr(TraitManager);
            ObjID arch = OBJ_NULL;
            if (obj != OBJ_NULL) {
               arch = pTraitManager->GetArchetype(obj);
            }
            *((NetObjID *) ptr) = arch;
            ptr += sizeof(NetObjID);
#endif
            break;
         }

         case kNMPT_Float:
            *((float *) ptr) = (float) va_arg(pArgs, double);
            ptr += sizeof(float);
            break;

         case kNMPT_Vector:
            *((mxs_vector *) ptr) = *(va_arg(pArgs, mxs_vector *));
            ptr += sizeof(mxs_vector);
            break;

         case kNMPT_Block:
         {
            uchar *pBlock = va_arg(pArgs, uchar *);
            memcpy(ptr, pBlock, pParam->size);
            ptr += pParam->size;
            break;
         }

         case kNMPT_DynBlock:
         {
            int size = va_arg(pArgs, int);
            *((ushort *) ptr) = size;
            ptr += sizeof(ushort);
            uchar *pBlock = va_arg(pArgs, uchar *);
            memcpy(ptr, pBlock, size);
            ptr += size;
            break;
         }

         case kNMPT_String:
         {
            const char *str = va_arg(pArgs, const char *);
            if (str == NULL) {
               // Just say that it's a null string
               *((uchar *) ptr) = NULL_STR_TOKEN;
               ptr += sizeof(uchar);
            } else {
               strcpy((char *) ptr, str);
               ptr += strlen(str) + 1;
            }
            break;
         }

         case kNMPT_MultiParm:
         case kNMPT_GlobalObjIDMultiParm:
         {
            const cMultiParm *pParm = va_arg(pArgs, const cMultiParm *);
            cMultiParm parm = *pParm;
            *((char *) ptr) = (char) parm.GetType();
            ptr += 1;
            switch (parm.GetType()) {
               case kMT_Undef:
                  // No value
                  break;
               case kMT_Int:
                  *((int *) ptr) = (int) parm;
                  ptr += sizeof(int);
                  break;
               case kMT_Obj:
               {
                  ObjID obj = (int) parm;
                  sGlobalObjID GID = gm_pNetMan->ToGlobalObjID(obj);
                  *((sGlobalObjID *) ptr) = GID;
                  ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
                  AutoAppIPtr(TraitManager);
                  ObjID arch = OBJ_NULL;
                  if (obj != OBJ_NULL) {
                     arch = pTraitManager->GetArchetype(obj);
                  }
                  *((NetObjID *) ptr) = arch;
                  ptr += sizeof(NetObjID);
#endif
                  break;
               }
               case kMT_Float:
                  *((float *) ptr) = (float) parm;
                  ptr += sizeof(float);
                  break;
               case kMT_String:
                  strcpy((char *) ptr, (const char *) parm);
                  ptr += strlen((const char *) parm) + 1;
                  break;
               case kMT_Vector:
                  *((mxs_vector *) ptr) = (mxs_vector) parm;
                  ptr += sizeof(mxs_vector);
                  break;
            }
            break;
         }
      }

      // Go on to the next parameter:
      pParam++;
   }

   // Record how big the message really is:
   Assert_(ptr > gm_pMsgBuffer);
   AssertMsg1((ptr < (gm_pMsgBuffer + MAX_MESSAGE_SIZE)),
              "Net Message %s is too long!",
              m_pDesc->msgName);
   *size = (ptr - gm_pMsgBuffer);

   return TRUE;
}

//////////
//
// MESSAGE RECEIVING
//

struct sNetMsg_Raw {
   sNetMsg_Generic base;
   uchar data[1];
};

// Check a proxy's archetype against one that was sent. This only happens
// in CHECK_ARCHETYPES builds.
void cNetMsg::CheckArchetypes(sGlobalObjID sentObj, 
                              ObjID myObj, ObjID sentArch)
{
   // First, check if the object exists at all:
   if (myObj == OBJ_NULL) {
      // Just ignore it
      return;
   }

   if (!gm_pObjSys->Exists(myObj)) {
      // We'll spew this out later. In most cases, we'll cut off the msg.
      return;
   }

   BOOL distinctAvatar = FALSE;
   // We allow certain specific types to get by without checking:
   if (!gDistinctAvatarProp->Get(myObj, &distinctAvatar) ||
       !distinctAvatar)
   {
      AutoAppIPtr(TraitManager);
      ObjID arch = OBJ_NULL;
      if (myObj != OBJ_NULL) {
         arch = pTraitManager->GetArchetype(myObj);
      }
      if (arch != sentArch)
      {
         // Sadly, ObjEditName uses a static buffer, so I can't just use it
         // twice in the assertion:
         char archName[128];
         strncpy(archName, ObjEditName(arch), 127);
         if (gm_bAssertArchetypes)
            CriticalMsg6("CHECKARCH %s: got obj %d (sent %d:%d), which I"
                         " think is a %s, but sender thinks is a %s!",
                         m_pDesc->msgName,
                         myObj,
                         sentObj.host,
                         sentObj.obj,
                         archName,
                         ObjEditName(sentArch));
         else
            // We want this to come out in opt builds. Note that
            // CheckArchetypes() won't get called at all in non-playtest
            // builds.
            mprintf("CHECKARCH %s: got obj %d (sent %d:%d), which I think"
                    " is a %s, but sender thinks is a %s!\n",
                    m_pDesc->msgName,
                    myObj,
                    sentObj.host,
                    sentObj.obj,
                    archName,
                    ObjEditName(sentArch));
      }
   }
}

//
// Raw message handler. Given a block of bytes from the netman, figure
// out what the heck to do with them.
//
void cNetMsg::HandleMsg(const sNetMsg_Generic *pGenericMsg, 
                        ulong msgSize, 
                        ObjID fromPlayer)
{
   sNetMsg_Raw *pMsg = (sNetMsg_Raw *) pGenericMsg;

   if (fromPlayer == OBJ_NULL)
   {
      // This message came through too early to know what player it's from;
      // it was likely sent just before a dbReset, or something. Only allow
      // it through if it's explicitly a MetagameBroadcast.
      if (!(m_pDesc->behaviour & kNMF_MetagameBroadcast)) {
#ifdef PLAYTEST
         if (m_bSpewMe) {
            const char *pName;
            if (m_pDesc->msgName) {
               pName = m_pDesc->msgName;
            } else if (m_pDesc->shortName) {
               pName = m_pDesc->shortName;
            } else {
               pName = "(unnamed)";
            }
            mprintf("%s: Got message from player with an ObjID\n", pName);
         }
#endif
         return;
      }
   }

   ObjID forwardTo;
   BOOL keepGoing = UnmarshalParams(pMsg->data, fromPlayer, &forwardTo);

   // Spew, if desired. This is unspeakably ugly, but efficient. It
   // should stay in synch with cfgdbg.h:
#ifdef PLAYTEST
   if (m_bSpewMe) {
      ReceiveSpew(fromPlayer);
   }
#endif

   if (!keepGoing)
   {
#ifdef PLAYTEST
      if (m_bSpewMe) {
         mprintf("  Message cut off due to non-existent object.\n");
      }
#endif
      return;
   }

   if ((forwardTo != OBJ_NULL) && (!(m_pDesc->behaviour & kNMF_NoForwarding)))
   {
      // *Sigh*; we've lost control of the object. Send the message
      // along to the real owner:
#ifdef PLAYTEST
      if (m_bSpewMe) {
         mprintf("  Target object handed off! Forwarding...\n");
      }
#endif
      AssertMsg2(gm_bNetworking,
                 "%s: Trying to forward msg to %s when not networking!",
                 m_pDesc->msgName,
                 ObjWarnName(forwardTo));
      ForwardMsg(forwardTo);
   } else {
      InterpretMsg(fromPlayer);
      if (m_pDesc->msgHandler) {
         // They specified an outside handler to deal with this message
         SendToHandler();
      }
   }

   // Clean up, if needed:
   ClearParams();
}

//
// Take the raw block of data, and figure out what parameters make it up.
//
BOOL cNetMsg::UnmarshalParams(uchar *ptr, ObjID fromPlayer, ObjID *pForwardTo)
{
   sNetMsgParam *pParam = &(m_pDesc->params[0]);
   *pForwardTo = OBJ_NULL;
   int i = 0;
   // This will become FALSE if we want to reject this message:
   BOOL keepGoing = TRUE;

   while (pParam->type != kNMPT_End) {
      switch(pParam->type) {
         case kNMPT_Int:
         case kNMPT_UInt:
            m_ppParams[i] = *((uint *) ptr);
            ptr += sizeof(uint);
            break;

         case kNMPT_Short:
         case kNMPT_UShort:
            m_ppParams[i] = *((ushort *) ptr);
            ptr += sizeof(ushort);
            break;

         case kNMPT_Byte:
         case kNMPT_UByte:
         case kNMPT_BOOL:
            m_ppParams[i] = *((uchar *) ptr);
            ptr += sizeof(uchar);
            break;

         case kNMPT_AbstractObjID:
         case kNMPT_ObjID:
            m_ppParams[i] = *((NetObjID *) ptr);
            ptr += sizeof(NetObjID);
            break;

         case kNMPT_ReceiverObjID:
         {
#ifdef CHECK_ARCHETYPES
            sGlobalObjID GID = *((sGlobalObjID *) ptr);
#endif
            ObjID obj = gm_pNetMan->FromGlobalObjID((sGlobalObjID *) ptr);
            m_ppParams[i] = obj;
            if (gm_pObjNet->ObjIsProxy(obj))
            {
               if (!gm_pObjSys->Exists(obj)) {
                  Warning(("Message sent to non-existent object %s!\n",
                           ObjEditName(obj)));
               } else {
                  if (m_pDesc->behaviour & kNMF_SendToObjOwner) {
                     // This message was sent to us because we used to
                     // own this object. However, it's been handed
                     // off, so forward the message to the new owner:
                     *pForwardTo = gm_pObjNet->ObjHostPlayer(obj);
                  }
               }
            }
            ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
            ObjID sentArch = *((NetObjID *) ptr);
            ptr += sizeof(NetObjID);
            CheckArchetypes(GID, m_ppParams[i], sentArch);
#endif
            if (obj != OBJ_NULL &&
                !gm_pObjSys->Exists(obj) &&
                !(pParam->flags & kNMPF_AllowNonExistentObj))
            {
               keepGoing = FALSE;
            }
            break;
         }
         case kNMPT_SenderObjID:
         case kNMPT_GlobalObjID:
         {
#ifdef CHECK_ARCHETYPES
            sGlobalObjID GID = *((sGlobalObjID *) ptr);
#endif
            ObjID obj = gm_pNetMan->FromGlobalObjID((sGlobalObjID *) ptr);
            m_ppParams[i] = obj;
            ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
            ObjID sentArch = *((NetObjID *) ptr);
            ptr += sizeof(NetObjID);
            CheckArchetypes(GID, obj, sentArch);
#endif
            if (obj != OBJ_NULL &&
                !gm_pObjSys->Exists(obj) &&
                !(pParam->flags & kNMPF_AllowNonExistentObj))
            {
               keepGoing = FALSE;
            }
            break;
         }

         case kNMPT_Float:
            // @NOTE: Does this look bizarre and strange? Bingo; you're
            // right! In order to shove the float through our DWORD-based
            // call mechanism without it getting truncated along the line,
            // we pretend that it's a uint, and count upon it getting
            // interpreted correctly by the callee. This particular hack
            // works only because floats are DWORD-sized.
            m_ppParams[i] = *((uint *) ptr);
            ptr += sizeof(float);
            break;

         case kNMPT_Vector:
            m_ppParams[i] = (DWORD) ptr;
            ptr += sizeof(mxs_vector);
            break;

         case kNMPT_Block:
         {
            m_ppParams[i] = (DWORD) ptr;
            ptr += pParam->size;
            break;
         }

         case kNMPT_DynBlock:
         {
            int size = *((ushort *) ptr);
            m_ppParams[i] = size;
            i++;
            ptr += sizeof(ushort);
            m_ppParams[i] = (DWORD) ptr;
            ptr += size;
            break;
         }

         case kNMPT_String:
         {
            if (*((uchar *) ptr) == NULL_STR_TOKEN) {
               // It was a null string
               m_ppParams[i] = NULL;
               ptr += sizeof(uchar);
            } else {
               m_ppParams[i] = (DWORD) ptr;
               ptr += strlen((const char *) m_ppParams[i]) + 1;
            }
            break;
         }

         case kNMPT_MultiParm:
         case kNMPT_GlobalObjIDMultiParm:
         {
            cMultiParm *parm = new cMultiParm();
            char parmtype = *((char *) ptr);
            ptr += sizeof(char);
            switch (parmtype) {
               case kMT_Undef:
                  break;
               case kMT_Int:
                  *parm = *((int *) ptr);
                  ptr += sizeof(int);
                  break;
               case kMT_Obj:
               {
#ifdef CHECK_ARCHETYPES
                  sGlobalObjID GID = *((sGlobalObjID *) ptr);
#endif
                  cObj obj;
                  obj.Set(gm_pNetMan->FromGlobalObjID((sGlobalObjID *)ptr));
                  *parm = obj;
                  ptr += sizeof(sGlobalObjID);
#ifdef CHECK_ARCHETYPES
                  ObjID sentArch = *((NetObjID *) ptr);
                  ptr += sizeof(NetObjID);
                  CheckArchetypes(GID, *parm, sentArch);
#endif
                  if (((int) *parm) != OBJ_NULL &&
                      !gm_pObjSys->Exists(*parm) &&
                      !(pParam->flags & kNMPF_AllowNonExistentObj))
                  {
                     keepGoing = FALSE;
                  }
                  break;
               }
               case kMT_Float:
                  *parm = *((float *) ptr);
                  ptr += sizeof(float);
                  break;
               case kMT_String:
                  *parm = (const char *) ptr;
                  ptr += strlen((const char *) parm) + 1;
                  break;
               case kMT_Vector:
                  *parm = *((mxs_vector *) ptr);
                  ptr += sizeof(mxs_vector);
                  break;
            }
            m_ppParams[i] = (DWORD) parm;
            break;
         }
      }

      // Go on to the next parameter:
      pParam++;
      i++;
   }

   // Append the player ID, if appropriate:
   if (m_pDesc->behaviour & kNMF_AppendSenderID) {
      m_ppParams[i] = fromPlayer;
   }

   return keepGoing;
}

//
// Clean up the marshalled params.
//
void cNetMsg::ClearParams()
{
   sNetMsgParam *pParam = &(m_pDesc->params[0]);
   int i = 0;

   while (pParam->type != kNMPT_End) {
      // Only types that actually need to be cleaned up should go in
      // here:  
      switch(pParam->type) {
         case kNMPT_MultiParm:
         case kNMPT_GlobalObjIDMultiParm:
         {
            cMultiParm *parm = (cMultiParm *) m_ppParams[i];
            delete parm;
         }
      }

      pParam++;
      i++;
   }
}

// Signatures for our possible message handlers. The largest of these should
// be one larger than MAX_DISPATCH:
typedef void (*tMsgFunc0)();
typedef void (*tMsgFunc1)(DWORD);
typedef void (*tMsgFunc2)(DWORD, DWORD);
typedef void (*tMsgFunc3)(DWORD, DWORD, DWORD);
typedef void (*tMsgFunc4)(DWORD, DWORD, DWORD, DWORD);
typedef void (*tMsgFunc5)(DWORD, DWORD, DWORD, DWORD, DWORD);
typedef void (*tMsgFunc6)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
typedef void (*tMsgFunc7)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
typedef void (*tMsgFunc8)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
typedef void (*tMsgFunc9)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);

//
// Send this message to an outside handler
//
// We only handle up to 8 params with this outside handler. For more,
// you'll need to implement InterpretMsg.
//
void cNetMsg::SendToHandler()
{
   if (m_pClientData) {
      switch(m_NumDispatchParams) {
         case 0:
            ((tMsgFunc1) m_pDesc->msgHandler)((DWORD) m_pClientData);
            break;
         case 1:
            ((tMsgFunc2) m_pDesc->msgHandler)(GetParam(0), 
                                              (DWORD) m_pClientData);
            break;
         case 2:
            ((tMsgFunc3) m_pDesc->msgHandler)(GetParam(0), 
                                              GetParam(1), 
                                              (DWORD) m_pClientData);
            break;
         case 3:
            ((tMsgFunc4) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              (DWORD) m_pClientData);
            break;
         case 4:
            ((tMsgFunc5) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              (DWORD) m_pClientData);
            break;
         case 5:
            ((tMsgFunc6) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              (DWORD) m_pClientData);
            break;
         case 6:
            ((tMsgFunc7) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              GetParam(5),
                                              (DWORD) m_pClientData);
            break;
         case 7:
            ((tMsgFunc8) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              GetParam(5),
                                              GetParam(6),
                                              (DWORD) m_pClientData);
            break;
         case 8:
            ((tMsgFunc9) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              GetParam(5),
                                              GetParam(6),
                                              GetParam(7),
                                              (DWORD) m_pClientData);
            break;
         default:
            Warning(("Too many params in msg to dispatch automatically.\n"));
      }
   } else {
      switch(m_NumDispatchParams) {
         case 0:
            ((tMsgFunc0) m_pDesc->msgHandler)();
            break;
         case 1:
            ((tMsgFunc1) m_pDesc->msgHandler)(GetParam(0));
            break;
         case 2:
            ((tMsgFunc2) m_pDesc->msgHandler)(GetParam(0), GetParam(1));
            break;
         case 3:
            ((tMsgFunc3) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2));
            break;
         case 4:
            ((tMsgFunc4) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3));
            break;
         case 5:
            ((tMsgFunc5) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4));
            break;
         case 6:
            ((tMsgFunc6) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              GetParam(5));
            break;
         case 7:
            ((tMsgFunc7) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              GetParam(5),
                                              GetParam(6));
            break;
         case 8:
            ((tMsgFunc8) m_pDesc->msgHandler)(GetParam(0),
                                              GetParam(1),
                                              GetParam(2),
                                              GetParam(3),
                                              GetParam(4),
                                              GetParam(5),
                                              GetParam(6),
                                              GetParam(7));
            break;
         default:
            Warning(("Too many params for SendToHandler!\n"));
            break;
      }
   }
}

//
// Forward this message to another player
//
// This *really* ought to be unified with SendToHandler, if I can ever
// figure out a way to do so...
//
// We actually do the forward simply by calling back into Send() again...
//
void cNetMsg::ForwardMsg(ObjID forwardTo)
{
   m_bForwarding = TRUE;

   switch(m_NumDispatchParams) {
      case 0:
         Send(OBJ_NULL);
         break;
      case 1:
         Send(OBJ_NULL,
              GetParam(0));
         break;
      case 2:
         Send(OBJ_NULL,
              GetParam(0), 
              GetParam(1));
         break;
      case 3:
         Send(OBJ_NULL,
              GetParam(0),
              GetParam(1),
              GetParam(2));
         break;
      case 4:
         Send(OBJ_NULL,
              GetParam(0),
              GetParam(1),
              GetParam(2),
              GetParam(3));
         break;
      case 5:
         Send(OBJ_NULL,
              GetParam(0),
              GetParam(1),
              GetParam(2),
              GetParam(3),
              GetParam(4));
         break;
      case 6:
         Send(OBJ_NULL,
              GetParam(0),
              GetParam(1),
              GetParam(2),
              GetParam(3),
              GetParam(4),
              GetParam(5));
         break;
      case 7:
         Send(OBJ_NULL,
              GetParam(0),
              GetParam(1),
              GetParam(2),
              GetParam(3),
              GetParam(4),
              GetParam(5),
              GetParam(6));
         break;
      case 8:
         Send(OBJ_NULL,
              GetParam(0),
              GetParam(1),
              GetParam(2),
              GetParam(3),
              GetParam(4),
              GetParam(5),
              GetParam(6),
              GetParam(7));
         break;
      default:
         Warning(("Too many params for SendToHandler!\n"));
         break;
   }

   m_bForwarding = FALSE;
}

//////////
//
// Loopback
//
void cNetMsg::DoLoopback(va_list pArgs)
{
   // Assemble the params into the params array:
   sNetMsgParam *pParam;
   int i;
   for (pParam = &(m_pDesc->params[0]), i = 0;
        pParam->type != kNMPT_End;
        pParam++, i++)
   {
      switch (pParam->type) {
         case kNMPT_MultiParm:
         case kNMPT_GlobalObjIDMultiParm:
         {
            const cMultiParm *pParm = va_arg(pArgs, const cMultiParm *);
            cMultiParm *parm = new cMultiParm(*pParm);
            m_ppParams[i] = (DWORD) parm;
            break;
         }
         case kNMPT_DynBlock:
            // DynBlocks get *two* params
            m_ppParams[i] = va_arg(pArgs, DWORD);
            i++;
            // FALL THROUGH
         default:
            m_ppParams[i] = va_arg(pArgs, DWORD);
            break;
      }
   }

   // Append the player ID, if appropriate:
   ObjID fromPlayer = PlayerObject();
   if (m_pDesc->behaviour & kNMF_AppendSenderID) {
      m_ppParams[i] = fromPlayer;
   }

   // Spew, if desired. This is unspeakably ugly, but efficient. It
   // should stay in synch with cfgdbg.h:
#ifdef PLAYTEST
   if (m_bSpewMe && config_is_defined("net_loopback_spew"))
   {
      mprintf("LOOPBACK: ");
      ReceiveSpew(fromPlayer);
   }
#endif

   // And now handle the message:
   InterpretMsg(fromPlayer);
   if (m_pDesc->msgHandler) {
      // Send to the outside handler
      SendToHandler();
   }

   // Clean up. For now, at least, this is consistently identical to
   // the normal handler case, so it works:
   ClearParams();
}

//////////
//
// Spews
//

// This prints out one line, describing the message to be sent
void cNetMsg::SendSpew(va_list pArgs)
{
   // The "header"
   const char *pName;
   if (m_pDesc->msgName) {
      pName = m_pDesc->msgName;
   } else if (m_pDesc->shortName) {
      pName = m_pDesc->shortName;
   } else {
      pName = "(unnamed)";
   }
   mprintf("SEND: %s --\n", pName);

   // Now iterate through all the parameters, and print each:
   int i = 0;
   sNetMsgParam *pParam = &(m_pDesc->params[0]);
   while (pParam->type != kNMPT_End) {
      if (pParam->flags & kNMPF_SuppressSpew) {
         pParam++;
         i++;
         continue;
      }

      // The name of the parameter:
      if (pParam->name) {
         mprintf("   %s: ", pParam->name);
      } else {
         mprintf("   param %d: ", i);
      }

      // Now the param itself:
      switch(pParam->type) {
         case kNMPT_Int:
         case kNMPT_UInt:
         {
            int n = va_arg(pArgs, uint);
            mprintf("%d", n);
            break;
         }
         case kNMPT_Short:
         case kNMPT_UShort:
         {
            int n = va_arg(pArgs, ushort);
            mprintf("%d", n);
            break;
         }
         case kNMPT_Byte:
         case kNMPT_UByte:
         {
            int n = va_arg(pArgs, uchar);
            mprintf("%d", n);
            break;
         }
         case kNMPT_BOOL:
         {
            BOOL b = va_arg(pArgs, BOOL);
            if (b) {
               mprintf("TRUE");
            } else {
               mprintf("FALSE");
            }
            break;
         }
         case kNMPT_SenderObjID:
         {
            int n = va_arg(pArgs, ObjID);
            mprintf("My Object [%s]", ObjEditName(n));
            break;
         }
         case kNMPT_ReceiverObjID:
         {
            // We have a proxy; translate it to the owner's lingo:
            ObjID proxy = va_arg(pArgs, ObjID);
            ObjID obj = gm_pObjNet->ObjHostObjID(proxy);
            ObjID objPlayer;
            if (gm_bNetworking) {
               objPlayer = gm_pObjNet->ObjHostPlayer(proxy);
            } else {
               // This had better be a loopback:
               objPlayer = PlayerObject();
            }
            mprintf("Proxy Object for %d:%d [%s]", 
                    objPlayer, obj, ObjEditName(proxy));
            break;
         }
         case kNMPT_AbstractObjID:
         case kNMPT_ObjID:
         {
            int n = va_arg(pArgs, ObjID);
            mprintf("Object [%s]", ObjEditName(n));
            break;
         }
         case kNMPT_GlobalObjID:
         {
            int n = va_arg(pArgs, ObjID);
            mprintf("Global Object [%s]", ObjEditName(n));
            break;
         }
         case kNMPT_Float:
         {
            float n = (float) va_arg(pArgs, double);
            mprintf("%g", n);
            break;
         }
         case kNMPT_Vector:
         {
            mxs_vector *pVec = va_arg(pArgs, mxs_vector *);
            mprintf("%g,%g,%g", pVec->x, pVec->y, pVec->z);
            break;
         }
         case kNMPT_Block:
         {
            va_arg(pArgs, char *);
            mprintf("(Block)");
            break;
         }
         case kNMPT_DynBlock:
         {
            va_arg(pArgs, int);
            va_arg(pArgs, char *);
            mprintf("(DynBlock)");
            break;
         }
         case kNMPT_String:
         {
            const char *str = va_arg(pArgs, const char *);
            if (str == NULL) {
               mprintf("(NULL string)");
            } else {
               mprintf("%s", str);
            }
            break;
         }
         case kNMPT_MultiParm:
         case kNMPT_GlobalObjIDMultiParm:
         {
            const cMultiParm *pParm = va_arg(pArgs, const cMultiParm *);
            cMultiParm parm = *pParm;
            switch (parm.GetType()) {
               case kMT_Undef:
                  mprintf("multi:(undef)");
                  // No value
                  break;
               case kMT_Int:
               {
                  int n = (int) parm;
                  mprintf("multi:int %d", n);
                  break;
               }
               case kMT_Obj:
               {
                  ObjID obj = (int) parm;
                  mprintf("multi:obj [%s]", 
                          ObjEditName(obj));
                  break;
               }
               case kMT_Float:
                  mprintf("multi:float %g", (float) parm);
                  break;
               case kMT_String:
                  mprintf("multi:str %s", (const char *) parm);
                  break;
               case kMT_Vector:
                  mprintf("multi:vec ()");
                  break;
            }
            break;
         }
      }

      // Go on to the next parameter:
      pParam++;
      i++;
      mprintf("\n");
   }
}

// This prints out one line, describing the message received
void cNetMsg::ReceiveSpew(ObjID fromPlayer)
{
   // The "header"
   const char *pName;
   if (m_pDesc->msgName) {
      pName = m_pDesc->msgName;
   } else if (m_pDesc->shortName) {
      pName = m_pDesc->shortName;
   } else {
      pName = "(unnamed)";
   }
   mprintf("RECEIVE from %d: %s --\n", fromPlayer, pName);

   // Now iterate through all the parameters, and print each:
   int i = 0;
   sNetMsgParam *pParam = &(m_pDesc->params[0]);
   while (pParam->type != kNMPT_End) {
      if (pParam->flags & kNMPF_SuppressSpew) {
         pParam++;
         i++;
         continue;
      }

      // The name of the parameter:
      if (pParam->name) {
         mprintf("   %s: ", pParam->name);
      } else {
         mprintf("   param %d: ", i);
      }

      // Now the param itself:
      switch(pParam->type) {
         case kNMPT_Int:
         case kNMPT_UInt:
         case kNMPT_Short:
         case kNMPT_UShort:
         case kNMPT_Byte:
         case kNMPT_UByte:
         {
            int n = (int) GetParam(i);
            mprintf("%d", n);
            break;
         }
         case kNMPT_BOOL:
         {
            BOOL b = (BOOL) GetParam(i);
            if (b) {
               mprintf("TRUE");
            } else {
               mprintf("FALSE");
            }
            break;
         }
         case kNMPT_SenderObjID:
         {
            mprintf("Proxy Object [%s]", ObjEditName(GetParam(i)));
            break;
         }
         case kNMPT_ReceiverObjID:
         {
            mprintf("My Object [%s]", ObjEditName(GetParam(i)));
            break;
         }
         case kNMPT_AbstractObjID:
         case kNMPT_ObjID:
         {
            mprintf("Object [%s]", ObjEditName(GetParam(i)));
            break;
         }
         case kNMPT_GlobalObjID:
         {
            mprintf("Global Object [%s]", ObjEditName(GetParam(i)));
            break;
         }
         case kNMPT_Float:
         {
            uint fi = GetParam(i);
            mprintf("%g", *((float *) &fi));
            break;
         }
         case kNMPT_Vector:
         {
            mxs_vector *pVec = (mxs_vector *) GetParam(i);
            mprintf("%g,%g,%g", pVec->x, pVec->y, pVec->z);
            break;
         }
         case kNMPT_Block:
         {
            mprintf("(Block)");
            break;
         }
         case kNMPT_DynBlock:
         {
            mprintf("(DynBlock)");
            break;
         }
         case kNMPT_String:
         {
            const char *str = (const char *) GetParam(i);
            if (str == NULL) {
               mprintf("(NULL string)");
            } else {
               mprintf("%s", str);
            }
            break;
         }
         case kNMPT_MultiParm:
         case kNMPT_GlobalObjIDMultiParm:
         {
            const cMultiParm parm = *((cMultiParm *) GetParam(i));
            switch (parm.GetType()) {
               case kMT_Undef:
                  mprintf("multi:(undef)");
                  // No value
                  break;
               case kMT_Int:
               {
                  int n = (int) parm;
                  mprintf("multi:int %d", n);
                  break;
               }
               case kMT_Obj:
               {
                  ObjID obj = (int) parm;
                  mprintf("multi:obj [%s]", ObjEditName(obj));
                  break;
               }
               case kMT_Float:
                  mprintf("multi:float %g", (float) parm);
                  break;
               case kMT_String:
                  mprintf("multi:str %s", (const char *) parm);
                  break;
               case kMT_Vector:
                  mprintf("multi:vec ()");
                  break;
            }
            break;
         }
      }

      // Go on to the next parameter:
      pParam++;
      i++;
      mprintf("\n");
   }
}
