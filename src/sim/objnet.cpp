/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objnet.cpp,v 1.46 1999/08/31 14:45:25 Justin Exp $
//
// Object Networking implementation
//
// This file assumes that cObjectNetworking will be instantiated only if
// networking in general is compiled in. Therefore, it does not have
// any NEW_NETWORK_ENABLED ifdefs.
//

#include <lg.h>
#include <appagg.h>
#include <constrid.h>
#include <aggmemb.h>
#include <config.h>
#include <cfgdbg.h>

#include <loopmsg.h>
#include <loopapi.h>  // for CreateSimpleLoopClient
#include <netloop.h>

#include <iobjsys.h>
#include <objdef.h>
#include <playrobj.h>
#include <linkbase.h>
#include <relation.h>
#include <lnkquery.h>

#include <wrtype.h>
#include <objpos.h>   // for gpCreateObjPosMsg

#include <dbfile.h>   // for dbCurrentFilenum()
#include <dbasemsg.h>
#include <dispatch.h>
#include <dispbase.h>

#include <hashpp.h>
#include <hshpptem.h>
#include <playrobj.h> 

#include <netman.h>
#include <netprops.h>
#include <netmsg.h>
#include <netnotif.h>

#include <netsynch.h>  // for NetSynchRemapMsgsDisabled()
#include <ghostapi.h>  // for GhostNotifyOwnerChanged()

#include <iobjnet_.h>

#include <objedit.h>   // for ObjEditName()

// Must be last header
#include <dbmem.h>

//////////
//
// ProxySpew
//
// Since proxification is the most delicate and important part of objnet,
// we want strong spewage for it. So we use this specialized macro (which
// works in opt builds) instead of ConfigSpew. Turn on net_proxy_spew to
// get lots of info about proxification.
//
#ifdef PLAYTEST
#define ProxySpew(msg) {if (config_is_defined("net_proxy_spew")) {mprintf msg;}}
#else
#define ProxySpew(msg)
#endif

//////////
//
// The HostObj relation
//
// In former times, this was a property; however, that fell afoul of the
// fact that you shouldn't store ObjIDs in properties. So it is now a
// relation. Each proxy object has a HostObj relation to the player who
// owns it; the data in that relation is the ObjID of the object on its
// host machine.
//

static IRelation *gpHostObjRel = NULL;

static void createHostObjRel(void)
{
   static sRelationDesc reldesc = { "HostObj", kRelationNetworkLocalOnly }; 
   static sRelationDataDesc datadesc = LINK_DATA_DESC(int);
   gpHostObjRel = CreateStandardRelation(&reldesc,&datadesc,0); 
}

static void closeHostObjRel(void)
{
   SafeRelease(gpHostObjRel);
}

// Accessors.
static BOOL GetHostObj(ObjID obj, ObjID *pHost, ObjID *pHostObjID)
{
   LinkID hostLink = gpHostObjRel->GetSingleLink(obj, LINKOBJ_WILDCARD);
   if (hostLink == LINKID_NULL) {
      // Doesn't have an explicit host:
      return FALSE;
   }
   sLink linkData;
   if (!gpHostObjRel->Get(hostLink, &linkData)) {
      Warning(("HostObj link is unreal?!?\n"));
      return FALSE;
   }
   *pHost = linkData.dest;
   *pHostObjID = *((int *) gpHostObjRel->GetData(hostLink));
   return TRUE;
}

static void SetHostObj(ObjID obj, ObjID host, ObjID hostObjID)
{
   ObjID oldHost, oldHostObjID;
   if (GetHostObj(obj, &oldHost, &oldHostObjID)) {
      if ((oldHost == host) &&
          (oldHostObjID == hostObjID))
      {
         // All is well
         return;
      } else {
         // Need to delete the old relation
         LinkID hostLink = gpHostObjRel->GetSingleLink(obj, LINKOBJ_WILDCARD);
         gpHostObjRel->Remove(hostLink);
      }
   }
   if (host == OBJ_NULL) {
      // We're deliberately removing the relation:
      ProxySpew(("PROXY: deproxifying %d\n", obj));
   } else {
      ProxySpew(("PROXY: object %d is a proxy for %d:%d\n",
                 obj, host, hostObjID));
      gpHostObjRel->AddFull(obj, host, &hostObjID);
   }
}

// Rebuild the proxy table based on the relations. We handle proxy table
// save/load this way, because the relation will deal with remappings
// properly, without any nasty intervention required.
void cObjectNetworking::RebuildProxyTable()
{
   ObjID me = PlayerObject();
   int filenum = dbCurrentFilenum();
   ProxySpew(("Rebuilding proxy table:\n"));
   // Iterate over all HostObj relations, and make sure that there is a
   // corresponding entry in the proxy table:
   ILinkQuery *pQuery = gpHostObjRel->Query(LINKOBJ_WILDCARD, 
                                            LINKOBJ_WILDCARD);
   if (pQuery != NULL) {
      for ( ; !pQuery->Done(); pQuery->Next())
      {
         int *pHostObjID = (int *) pQuery->Data();
         AssertMsg(pHostObjID != NULL,
                   "HostObj relation without a HostObjID!");
         int HostObjID = *pHostObjID;
         sLink link;
         pQuery->Link(&link);
         if ((me != OBJ_NULL) && 
             (link.dest == me))
         {
            // This is a local "proxy", probably an item in our inventory.
            // Make sure it is mapped properly.
            if (HostObjID != link.source) {
               // The object's been remapped, so fix it:
               ProxySpew(("Fixing local proxy: %d --> %d\n",
                          HostObjID,
                          link.source));
               HostObjID = link.source;
               SetHostObj(link.source, link.dest, HostObjID);
            }
         }
         ProxySpew(("   proxy %d is really %d:%d\n",
                    link.source,
                    link.dest,
                    HostObjID));
         // Create the proxy entry
         SetProxyTableEntry(link.dest, HostObjID, link.source);

         // Now the really tricky bit. If this was a remapped object,
         // we may need to munge the remapping tables:
         sProxyFileObjID proxyObjID = {filenum, link.source};
         ObjID oldObjID;
         if (mpProxyRemapTable->Lookup(&proxyObjID, &oldObjID))
         {
            // Okay, this proxy was remapped. Kill the simpleminded
            // remap entry:
            sFileObjID *fileObjID = new sFileObjID;
            fileObjID->filenum = filenum;
            fileObjID->owner = OBJ_NULL;
            fileObjID->oldHostObjID = oldObjID;
            sFileObjID *oldKey = mpRemapTable->GetKey(fileObjID);
            mpRemapTable->Delete(fileObjID);
            delete oldKey;
            delete fileObjID;
         }

         // Finally, put an entry into the remap table, just to be on the
         // safe side. This lets us know exactly which file this entry
         // actually comes from, and helps disambiguate things. It's a
         // slight misnomer, since we haven't necessarily remapped this
         // object, but we're recording what we think the object's mapping
         // is for each file-object.
         sFileObjID *fileObjID = new sFileObjID;
         fileObjID->filenum = filenum;
         fileObjID->owner = link.dest;
         fileObjID->oldHostObjID = HostObjID;
         if (mpRemapTable->HasKey(fileObjID))
         {
            // This thing's already in the table. Kill the old entry,
            // to be safe:
            sFileObjID *oldKey = mpRemapTable->GetKey(fileObjID);
            mpRemapTable->Delete(oldKey);
            delete oldKey;
         }
         mpRemapTable->Set(fileObjID, link.source);
      }
   }
   SafeRelease(pQuery);
   ProxySpew(("Finished rebuilding proxy table\n"));
}

////////////////////////////////////////////////////////////
// cObjectNetworking
//

F_DECLARE_INTERFACE(IObjectNetworking);

cObjectNetworking *g_pObjNet;

//------------------------------------------------------------
// Construction/Deconstruction 
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cObjectNetworking);

static sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_IObjectSystem },
   { kConstrainAfter, &IID_INetManager },
   { kNullConstraint}, 
};

cObjectNetworking::cObjectNetworking(IUnknown* pOuter)
   : mpNetMan(NULL),
     mbHandlingProxy(FALSE),
     mObjBeingProxied(OBJ_NULL),
     mCreationDepth(0),
     mbNetworking(FALSE),
     mbNetworkGame(FALSE),
     mLocalOnlies(0),
     mObjBeingCreated(OBJ_NULL)
{
   INIT_AGGREGATION_1(pOuter,IID_IObjectNetworking,this,kPriorityNormal,Constraints);
}

cObjectNetworking::~cObjectNetworking()
{
}

//////////
//
// Object System Listener
//
// We use this to keep track of object creation and deletion.
//
static void objSysListener(ObjID obj, eObjNotifyMsg msg, void *)
{
   g_pObjNet->ObjSysListener(obj, msg);
}

void cObjectNetworking::ObjSysListener(ObjID obj, eObjNotifyMsg msg)
{
   switch(msg) {
      case kObjNotifyDelete:
         // They're deleting an object. If it's a proxy, we need to
         // keep track of it:
         if (ObjIsProxy(obj)) {
            ProxySpew(("PROXY: Proxy %d being deleted.\n", obj));
            mpDeletedProxyTable->Insert(obj, TRUE);
         }
         break;
   }
}

// Listener for network messages. Note that this is static, since it's
// a callback.
void cObjectNetworking::NetworkListener(eNetListenMsgs situation,
                                        DWORD /* data */,
                                        void *pClientData)
{
   cObjectNetworking *pObjNet = (cObjectNetworking *) pClientData;
   switch (situation)
   {
      case kNetMsgNetworking:
#ifdef PLAYTEST
         if (config_is_defined("net_check_remaps"))
            pObjNet->SendRemapChecks();
#endif
         break;
      // We live a tad dangerously, and accept messages during synch mode,
      // because there is a brief (but sometimes significant) delay
      // between getting a player's create message and the start of
      // networking mode...
      case kNetMsgSynchronize:
         pObjNet->mbNetworking = TRUE;
         break;
      case kNetMsgReset:
         pObjNet->mbNetworking = FALSE;
         break;
      case kNetMsgNetworkGame:
         pObjNet->mbNetworkGame = TRUE;
         break;
      case kNetMsgNetworkLost:
         pObjNet->mbNetworkGame = FALSE;
         break;
   }
}

//////////////////////////////
//
// NETWORK MESSAGES
//

//////////
//
// Request Destroy
//
// This message asks the owner of an object to please destroy it.
//
static void handleRequestDestroy(ObjID obj)
{
   AutoAppIPtr(ObjectSystem);
   // Guard against the possibility that this thing's been destroyed
   // already; can happen if the machines are doing something in parallel:
   if (pObjectSystem->Exists(obj))
      pObjectSystem->Destroy(obj);
}

static sNetMsgDesc sRequestDestroyDesc = {
   kNMF_SendToObjOwner,
   "ReqDestroy",
   "Request Object Destroy",
   NULL,
   handleRequestDestroy,
   {{kNMPT_ReceiverObjID},
    {kNMPT_End}}
};

STDMETHODIMP_(void) cObjectNetworking::RequestDestroy(ObjID obj)
{
   mpRequestDestroyMsg->Send(OBJ_NULL, obj);
}

//////////
//
// Object Position
//
// This message tells us what the object's position is, as part of
// creation of proxies.
//
static void handleCreateObjPos(ObjID obj, 
                               mxs_vector *location,
                               mxs_angvec *facing)
{
   ObjPosUpdate(obj, location, facing);
}

static sNetMsgDesc sCreateObjPosDesc = {
   kNMF_Broadcast,
   "CreateObjPos",
   "Create Obj Pos",
   NULL,
   handleCreateObjPos,
   {{kNMPT_SenderObjID, kNMPF_None, "Object"},
    {kNMPT_Vector, kNMPF_None, "Location"},
    {kNMPT_Block, kNMPF_None, "Facing", sizeof(mxs_angvec)},
    {kNMPT_End}}
};

//////////
//
// Begin Create
//
// This message gets sent to everyone else when we create a non-local
// object.
//

void cObjectNetworking::HandleBeginCreate(ObjID arch, ObjID obj, ObjID from)
{
   // A remote system created an object that we might care about.
   // Create a proxy to stand in for it

   // Note that the protocol here is a tad complex. We call BeginCreate,
   // which creates the raw object, and then immediately calls
   // BroadcastBeginCreate. There, we detect that we're creating a
   // proxy, and register it immediately, so that any *other* listeners
   // on BeginCreate find out that it's a proxy.
   mObjBeingProxied = obj;
   mRealObjOwner = from;
   ObjID proxy = mpObjSys->BeginCreate(arch, kObjectConcrete);
   mObjBeingProxied = OBJ_NULL;
   mRealObjOwner = OBJ_NULL;
}

static void handleBeginCreate(ObjID exemplar, ObjID obj, ObjID from)
{
   g_pObjNet->HandleBeginCreate(exemplar, obj, from);
}

static sNetMsgDesc sBeginCreateDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "BeginCreate",
   "Begin Create",
   NULL,
   handleBeginCreate,
   {{kNMPT_GlobalObjID, kNMPF_None, "Exemplar"},
    {kNMPT_ObjID, kNMPF_None, "Obj"},
    {kNMPT_End}}
};

STDMETHODIMP_(void) cObjectNetworking::StartBeginCreate
  (ObjID exemplar, 
   ObjID obj)
{
   if (!mbNetworking)
   {
      // We keep track of the object currently being created, for one
      // or two special cases:
      mObjBeingCreated = obj;
      return;
   }

   // Empty out any old entries pointing to this objnum; this needs to
   // happen now, before we go creating any new entry:
   ClearProxyEntry(obj);

   // This is a hateful but practical bit of hidden semantics. If
   // we begin creating an object in a networked game, but networking
   // is suspended, then this had damned well *better* be a local-only
   // object, or we're gonna cause 97 kinds of confusion. It's also
   // a convenient backdoor for creating local-only objects on the fly.
   BOOL madeLocal = FALSE;
   if (mpNetMan->Suspended()) {
      SetNetworkCategory(obj, kLocalOnly);
      madeLocal = TRUE;
   }

   mCreationDepth++;
   if (mCreationDepth > 1) {
      // Don't do anything at this point; we'll make the thing local
      // down in FinishBeginCreate.
   } else if (mObjBeingProxied != OBJ_NULL) {
      // This is creating a proxy, so it shouldn't be broadcast, but
      // we should register the proxy now

      // And register the relationship
      ObjRegisterProxy(mRealObjOwner, mObjBeingProxied, obj);
      ConfigSpew("net_spew", 
                 ("Registered proxy %d for remote %d:%d\n",
                  obj, mRealObjOwner, mObjBeingProxied));
      mObjBeingProxied = OBJ_NULL;
   } else if ((NetworkCategory(exemplar) != kLocalOnly) && !madeLocal) {
      // The object is only being created here, and will be hosted here.
      // Register it here, with the proxy obj id = the host obj id.
      ObjRegisterProxy(PlayerObject(), obj, obj);

      // Now create a message to the other players, indicating that this
      // object is being created.
      mpBeginCreateMsg->Send(OBJ_NULL, exemplar, obj);
   }
   // Now suspend networking for a bit, while the object actually
   // gets created:
   mpNetMan->SuspendMessaging();
}

STDMETHODIMP_(void) cObjectNetworking::FinishBeginCreate(ObjID obj)
{
   if (!mbNetworking)
      return;

   // If this object is a side-effect, then mark it local-only:
   if (mCreationDepth > 1) {
      SetNetworkCategory(obj, kLocalOnly);
   }

   if (NetworkCategory(obj) == kLocalOnly) {
      mLocalOnlies++;
   }

   // Re-enable networking
   mpNetMan->ResumeMessaging();

   mCreationDepth--;
}

//////////
//
// End Create
//
// This message gets sent to other players when we finish creating an
// object.
//
void cObjectNetworking::HandleEndCreate(ObjID obj)
{
   // The remote system has finished creating the object; we should
   // by now have received all of the initial property assignments
   // for the object.

   // don't broadcast create/delete of the proxy
   mbHandlingProxy = TRUE;
   mpObjSys->EndCreate(obj);
   mbHandlingProxy = FALSE;
}

static void handleEndCreate(ObjID obj)
{
   g_pObjNet->HandleEndCreate(obj);
}

static sNetMsgDesc sEndCreateDesc =
{
   kNMF_Broadcast,
   "EndCreate",
   "End Create",
   NULL,
   handleEndCreate,
   {{kNMPT_SenderObjID, kNMPF_AllowObjNull, "Obj"},
    {kNMPT_End}}
};

STDMETHODIMP_(void) cObjectNetworking::StartEndCreate(ObjID obj)
{
   if (!mbNetworking)
      return;

   mCreationDepth++;

   mpNetMan->SuspendMessaging();
}

STDMETHODIMP_(void) cObjectNetworking::FinishEndCreate(ObjID obj)
{
   if (!mbNetworking)
   {
      // We keep track of the object currently being created, for one
      // or two special cases:
      mObjBeingCreated = OBJ_NULL;
      return;
   }

   mpNetMan->ResumeMessaging();

   if (!mbHandlingProxy)
   {
      if (!mpObjSys->Exists(obj)) {
         if (mLocalOnlies == 0) {
            // This object's already been destroyed; send a "noop" EndCreate,
            // just to unlock things:
            mpEndCreateMsg->Send(OBJ_NULL, OBJ_NULL);
         }
      } else if (NetworkCategory(obj) != kLocalOnly) {
         ObjPos *pPos = ObjPosGet(obj);
         if (pPos) {
            mpCreateObjPosMsg->Send(OBJ_NULL, 
                                    obj, 
                                    &(pPos->loc.vec), 
                                    &(pPos->fac));
         }

         // Now, send the EndCreate itself:
         mpEndCreateMsg->Send(OBJ_NULL, obj);
      }
   }
   if (mLocalOnlies) {
      // We count on the fact that, if the object creations wind up
      // nesting, anything below a local-only will also be local-only:
      mLocalOnlies--;
   }
   mCreationDepth--;
}

//////////
//
// Destroy Object
//
void cObjectNetworking::HandleDestroy(ObjID obj)
{
   // Don't delete the proxy info, in case there are old net messages
   // outstanding.

   // Make sure we really have a proxy for this; otherwise, we can
   // wind up deleting completely the wrong thing:
   if (!mpObjSys->Exists(obj)) {
      Warning(("I've been told to delete bogus object %d; refusing!\n", obj));
      return;
   }
   AssertMsg1(ObjIsProxy(obj),
              "I've been told to delete non-proxy object %s!", ObjWarnName(obj));

   // Destroy the proxy object.
   mbHandlingProxy = TRUE;
   mpObjSys->Destroy(obj);
   mbHandlingProxy = FALSE;
}

static void handleDestroy(ObjID obj)
{
   g_pObjNet->HandleDestroy(obj);
}

static sNetMsgDesc sDestroyDesc =
{
   kNMF_Broadcast,
   "Destroy",
   "Destroy Object",
   NULL,
   handleDestroy,
   {{kNMPT_SenderObjID, kNMPF_IfHostedHere, "Obj"},
    {kNMPT_End}}
};

STDMETHODIMP_(void) cObjectNetworking::StartDestroy(ObjID obj)
{
   if (!mbNetworking)
      return;

   // Don't send a message if we don't even believe this thing exists:
   if (mpObjSys->Exists(obj))
      mpDestroyMsg->Send(OBJ_NULL, obj);

   // Don't delete the proxy info, since a lingering net message
   // may want to refer to this object, and we want to correctly
   // map onto the right objId, even though it has been destroyed.

   // @TBD: Do we need this any more?
   mpNetMan->SuspendMessaging();
}

STDMETHODIMP_(void) cObjectNetworking::FinishDestroy(ObjID obj)
{
   if (!mbNetworking)
      return;

   // @TBD: Do we need this any more?
   mpNetMan->ResumeMessaging();
}

//////////
//
// New Owner
//
// This message indicates that the given object now has a new owner. It is
// a particularly crucial message for system consistency, so we put some
// extra assertion weight on it.
//
void cObjectNetworking::HandleNewOwner(ObjID proxy, 
                                       ObjID newHostObjID,
                                       ObjID newOwner,
                                       int serial)
{
   AssertMsg3(NetworkCategory(proxy) == kHosted,
              "Got NewOwner msg for local-only object %s; now %d:%d!",
              ObjEditName(proxy), newOwner, newHostObjID);

   // Check the serial number for the handoff, and ignore it if it's
   // old:
   int prevSerial = 0;
   gHandoffNumberProp->Get(proxy, &prevSerial);
   if (serial < prevSerial)
   {
      Warning(("NewOwner message received out of order; ignoring...\n"));
      return;
   }
   // Real serial numbers should always be greater than 0, and should
   // never repeat, so:
   AssertMsg2(serial != prevSerial,
              "NewOwner serial number %d for object %s reused!",
              serial,
              ObjWarnName(proxy));
   gHandoffNumberProp->Set(proxy, serial);

   // Now register the new owner's proxy info.
   // NOTE: Don't delete the old proxy info, since there may be
   // outstanding messages on the way regarding this object, using its
   // old host.
   ObjRegisterProxy(newOwner, newHostObjID, proxy);

   // Tell the ghost system that ownership has changed.
   // @TBD: turn this and all the other calls into a proper listener
   // mechanism
   GhostNotifyOwnerChanged(proxy, (newOwner == PlayerObject()));
}

static void handleNewOwner(ObjID proxy, ObjID newHostObjID, 
                           ObjID newOwner, int serial)
{
   g_pObjNet->HandleNewOwner(proxy, newHostObjID, newOwner, serial);
}

static sNetMsgDesc sNewOwnerDesc =
{
   kNMF_Broadcast,
   "NewOwner",
   "New Owner for Object",
   NULL,
   handleNewOwner,
   {{kNMPT_GlobalObjID, kNMPF_None, "Old ObjID"},
    {kNMPT_ObjID, kNMPF_None, "New ObjID"},
    {kNMPT_GlobalObjID, kNMPF_None, "New Owner"},
    {kNMPT_Int, kNMPF_None, "Handoff Serial Number"},
    {kNMPT_End}}
};

void cObjectNetworking::BroadcastNewOwner(ObjID obj,
                                          ObjID newHost,
                                          ObjID newObjID)
{
   if (!mbNetworking) {
      // We're not networking; that had better be because this simply isn't
      // a network game:
      AssertMsg3(!mpNetMan->IsNetworkGame(),
                 "Trying to send NewOwner %d of %d (formerly %d) when not networking!",
                 newHost,
                 newObjID,
                 obj);
      return;
   }

   // This message absolutely must not be sent when we're suspended:
   AssertMsg3(!mpNetMan->Suspended(),
              "Trying to send NewOwner %d of %d (formerly %d) when suspended!",
              newHost,
              newObjID,
              obj);

   if (NetworkCategory(obj) != kHosted) {
      Warning(("Attempt to change ownership of local obj %d!\n", obj));
      return;
   }

   // Figure out the serial number for this handoff:
   int serial = 0;
   gHandoffNumberProp->Get(obj, &serial);
   serial++;
   gHandoffNumberProp->Set(obj, serial);

   // Broadcast the new owner and objid for this object.  Note this
   // assumes that the old host info still exists for obj.
   mpNewOwnerMsg->Send(OBJ_NULL, obj, newObjID, newHost, serial);
}

//////////
//
// Remap Object
//
// This rather complex bit of machinery gets called at load time, if we
// have to remap objects due to objnum conflicts. It deals with keeping
// proxies in synch with each other.
//
// The guts of the remapping code is further down this file.
//

static void handleRemap(uchar playerNum,
                        uint remapSize,
                        sSingleRemap *remaps)
{
   g_pObjNet->HandleRemap(playerNum, remapSize, remaps);
}

static sNetMsgDesc sRemapDesc =
{
   kNMF_MetagameBroadcast,
   "Remap",
   "Remap Object",
   NULL,
   handleRemap,
   {{kNMPT_UByte, kNMPF_None, "Player Number"},
    {kNMPT_DynBlock, kNMPF_None, "Remappings"},
    {kNMPT_End}}
};

//////////
//
// Check Remap
//
// This is a stupid, trivial little message that exists solely for
// debugging. It actually just exercises code inside cNetMsg to check
// object consistency.
//
static void handleCheckRemap(ObjID obj)
{
   // We don't actually give a damn about this message; the interesting
   // checks have already been done.
}

static sNetMsgDesc sCheckRemapDesc =
{
   kNMF_Broadcast,
   "CheckRemap",
   "Check Object Remappings",
   NULL,
   handleCheckRemap,
   {{kNMPT_GlobalObjID, kNMPF_None, "Obj"},
    {kNMPT_End}}
};

//////////
//
// Give an object away imperitively
//
// This is the beginning of a tricky little protocol. We send this
// message off to the target player, and locally mark the object as
// owned by him, but with an unknown ObjID. When he gets the message,
// he does an ObjTakeOver. In the meantime, the original host should
// forward any message destined for this object, using its own old
// objID to refer to it.
//
// Related code is scattered around, in NetMan->ToGlobalObjID, as
// well as in the forwarding code in cNetMsg.

static void handleGiveObject(ObjID obj)
{
   // I'm the recipient, and I've been told to take this thing, so
   // go grab it:
   g_pObjNet->ObjTakeOver(obj);
};

static sNetMsgDesc sGiveObjectDesc =
{
   kNMF_None,
   "GiveObject",
   "Give Away Object",
   NULL,
   handleGiveObject,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_End}}
};

STDMETHODIMP_(void) cObjectNetworking::ObjGiveWithoutObjID(ObjID obj,
                                                           ObjID player)
{
   if (player == PlayerObject())
      // Give it to ourselves. Duh.
      return;

   mpGiveObjectMsg->Send(player, obj);
   ObjRegisterProxy(player, OBJ_NULL, obj);
   // Tell the ghost system that we don't own it any more:
   GhostNotifyOwnerChanged(obj, FALSE);
}

//------------------------------------------------------------
// Init/Shutdown
//

void dispatchObjSysMsg(const sNetMsg_Generic *pMsg, 
                       ulong size,
                       ObjID from, 
                       void *pClientData);

HRESULT cObjectNetworking::Init()
{
   mpNetMan = AppGetObj(INetManager);
   mpObjSys = AppGetObj(IObjectSystem);

   createHostObjRel();

   // @TBD: This should probably be initialized to approximately the number
   // of proxy objects we expect to handle:
   mpProxyTable = new cHashByHostObjID;
   mpRemapTable = new cHashByFileObjID;
   mpProxyRemapTable = new cProxyRemapTable;
   mpDeletedProxyTable = new cProxyListHash;

   // Set up the cNetMsgs:
   mpRequestDestroyMsg = new cNetMsg(&sRequestDestroyDesc);
   mpCreateObjPosMsg = new cNetMsg(&sCreateObjPosDesc);
   mpBeginCreateMsg = new cNetMsg(&sBeginCreateDesc);
   mpEndCreateMsg = new cNetMsg(&sEndCreateDesc);
   mpDestroyMsg = new cNetMsg(&sDestroyDesc);
   mpNewOwnerMsg = new cNetMsg(&sNewOwnerDesc);
   mpRemapMsg = new cNetMsg(&sRemapDesc);
   mpCheckRemapMsg = new cNetMsg(&sCheckRemapDesc);
   mpGiveObjectMsg = new cNetMsg(&sGiveObjectDesc);

   g_pObjNet = this;

   // Keep track of the networking state:
   mpNetMan->Listen(NetworkListener, 
                    (kNetMsgNetworking | kNetMsgReset | kNetMsgSynchronize |
                     kNetMsgNetworkGame | kNetMsgNetworkLost),
                    this);
   // In case netman's already decided on the network game:
   mbNetworkGame = mpNetMan->IsNetworkGame();

   // We need to pay some attention to what objects are getting created
   // and deleted, to track the proxies appropriately:
   sObjListenerDesc listenDesc = {objSysListener, this};
   mpObjSys->Listen(&listenDesc);

   return S_OK;
}

HRESULT cObjectNetworking::End()
{
   // @TBD: Are the *contents* of these tables ever getting properly
   // deleted?
   if (mpProxyTable) {
      cHashByHostObjID::cIter iter;
      for (iter = mpProxyTable->Iter();
           !iter.Done();
           iter.Next())
      {
         sHostObjID *key = iter.Key();
         mpProxyTable->Delete(key);
         delete key;
      }
      delete mpProxyTable;
   }
   if (mpRemapTable) {
      cHashByFileObjID::cIter iter;
      for (iter = mpRemapTable->Iter();
           !iter.Done();
           iter.Next())
      {
         sFileObjID *key = iter.Key();
         mpRemapTable->Delete(key);
         delete key;
      }
      delete mpRemapTable;
   }
   if (mpProxyRemapTable) {
      cProxyRemapTable::cIter iter;
      for (iter = mpProxyRemapTable->Iter();
           !iter.Done();
           iter.Next())
      {
         sProxyFileObjID *key = iter.Key();
         mpProxyRemapTable->Delete(key);
         delete key;
      }
      delete mpProxyRemapTable;
   }
   if (mpDeletedProxyTable) {
      delete mpDeletedProxyTable;
   }

   closeHostObjRel();

   SafeRelease(mpNetMan);
   SafeRelease(mpObjSys);

   // Delete the cNetMsgs:
   delete mpRequestDestroyMsg;
   delete mpCreateObjPosMsg;
   delete mpBeginCreateMsg;
   delete mpEndCreateMsg;
   delete mpDestroyMsg;
   delete mpNewOwnerMsg;
   delete mpRemapMsg;
   delete mpCheckRemapMsg;
   delete mpGiveObjectMsg;

   g_pObjNet = NULL;

   return S_OK;
}

//
// Clear everything out. Generally called when we reset the database back
// to empty.
//
STDMETHODIMP_(void) cObjectNetworking::ClearTables()
{
   if (mpProxyTable) {
      cHashByHostObjID::cIter iter;
      for (iter = mpProxyTable->Iter();
           !iter.Done();
           iter.Next())
      {
         sHostObjID *key = iter.Key();
         mpProxyTable->Delete(key);
         delete key;
      }
      delete mpProxyTable;
   }
   mpProxyTable = new cHashByHostObjID;

   if (mpRemapTable) {
      cHashByFileObjID::cIter iter;
      for (iter = mpRemapTable->Iter();
           !iter.Done();
           iter.Next())
      {
         sFileObjID *key = iter.Key();
         mpRemapTable->Delete(key);
         delete key;
      }
      delete mpRemapTable;
   }
   mpRemapTable = new cHashByFileObjID;
   if (mpProxyRemapTable) {
      cProxyRemapTable::cIter iter;
      for (iter = mpProxyRemapTable->Iter();
           !iter.Done();
           iter.Next())
      {
         sProxyFileObjID *key = iter.Key();
         mpProxyRemapTable->Delete(key);
         delete key;
      }
      delete mpProxyRemapTable;
   }
   mpProxyRemapTable = new cProxyRemapTable;
   if (mpDeletedProxyTable) {
      delete mpDeletedProxyTable;
   }
   mpDeletedProxyTable = new cProxyListHash;
}

STDMETHODIMP_(BOOL) cObjectNetworking::HandlingProxy()
{
   return mbHandlingProxy;
}

// 
// This routine is for taking over an object from some other player to
// this one.
STDMETHODIMP_(void) cObjectNetworking::ObjTakeOver(ObjID obj)
{
   // Get our old proxy for the object, which will now become the
   // "official" version of this object:
   ConfigSpew("net_spew", 
              ("OBJNET: taking over %d from %d\n", obj, ObjHostPlayer(obj)));

   // Tell the rest of the world that we now own it (must be done
   // before changing proxy info).
   BroadcastNewOwner(obj, PlayerObject(), obj);

   // Now register the new owner's proxy info.
   // NOTE: Don't delete the proxy info, since there may be
   // outstanding messages on the way regarding this object, using its
   // old host.
   ObjRegisterProxy(PlayerObject(), obj, obj);

   // Tell the ghost system about the change
   GhostNotifyOwnerChanged(obj, TRUE);
}

//
// This routine is for giving an object to some other player.
STDMETHODIMP_(void) cObjectNetworking::ObjGiveTo(ObjID obj, 
                                                 ObjID player,
                                                 ObjID newObjID)
{
   ConfigSpew("net_spew", 
              ("OBJNET: giving %d to %d:%d\n", obj, player, newObjID));

   AssertMsg1(ObjHostedHere(obj),
              "Trying to give away object %s that I don't own!",
              ObjWarnName(obj));

   if (player == PlayerObject())
   {
      Warning(("I'm giving %s to myself; why?\n", ObjWarnName(obj)));
      return;
   }

   // Tell the rest of the world that we now own it (must be done
   // before changing proxy info).
   BroadcastNewOwner(obj, player, newObjID);

   // Now register the new owner's proxy info.
   // NOTE: Don't delete the proxy info, since there may be
   // outstanding messages on the way regarding this object, using its
   // old host.
   ObjRegisterProxy(player, newObjID, obj);

   // Tell the ghost system:
   GhostNotifyOwnerChanged(obj, FALSE);
}

//////////////////////////////////////////////////////////
//
// General Information about Objects
//
// These methods control and provide information on where an object
// "lives" (that is, which machine owns it).
//

void cObjectNetworking::SetProxyTableEntry
  (ObjID hostPlayer, 
   NetObjID objIdOnHost,
   ObjID proxy)
{
   // Allocate the key that will be used in the table.
   sHostObjID *hostObjID = new sHostObjID;
   hostObjID->host = hostPlayer;
   hostObjID->netObjID = objIdOnHost;

   // Get the old key, if there was one:
   BOOL hasOldKey = mpProxyTable->HasKey(hostObjID);
   sHostObjID *oldHostObjID = NULL;
   if (hasOldKey)
      oldHostObjID = mpProxyTable->GetKey(hostObjID);

   // We use Set instead of Insert, since an old entry may still exist
   // for hostObjID (we never delete them).
   mpProxyTable->Set(hostObjID, proxy);
   if (hasOldKey)
      delete oldHostObjID;
}

//
// Proxy Management
//
// These three methods deal with the "proxy" objects that stand in for
// "real" objects on other machines. It basically manages a mapping from
// a remote objID to its representation locally.
//
STDMETHODIMP_(void) cObjectNetworking::ObjRegisterProxy
  (ObjID hostPlayer, 
   NetObjID objIdOnHost,
   ObjID proxy)
{
   AssertMsg(mpProxyTable, "No Proxy ObjID table!");

   if ((hostPlayer != OBJ_NULL) && (objIdOnHost != OBJ_NULL))
   {
      // Set up the relation for host->proxy lookups...
      SetProxyTableEntry(hostPlayer, objIdOnHost, proxy);
   }
   // ... and proxy->host lookups.
   SetHostObj(proxy, hostPlayer, objIdOnHost);
}

STDMETHODIMP_(ObjID) cObjectNetworking::ObjGetProxy
  (ObjID hostPlayer, 
   NetObjID objIdOnHost)
{
   AssertMsg(mpProxyTable, "No Proxy ObjID table!");

   sHostObjID hostObjID = {hostPlayer, objIdOnHost};

   ObjID result;
   ObjID checkHost;
   ObjID checkHostObjID;
   if (mpProxyTable->Lookup(&hostObjID, &result)) 
   {
#ifdef PLAYTEST
      if (!GetHostObj(result, &checkHost, &checkHostObjID) ||
          (hostPlayer != checkHost) ||
          (objIdOnHost != checkHostObjID))
      {
         // Okay -- the property doesn't match the proxy table. This
         // might mean that the object has been rehosted, or that the
         // proxy has been deleted and reused.
         // @TBD: once we're completely confident that the proxy table
         // is entirely solid, take out this warning.
         ProxySpew(("ObjGetProxy inconsistent (rehosted?). "
                    "Proxy: %d; found from %d:%d; now %d:%d\n",
                    result, hostPlayer, objIdOnHost,
                    checkHost, checkHostObjID));
      }
      // Return our best guess of the right object:
#endif
      return result;
   } else {
      // There is no mapping, so we assume that it is the same ObjID
      // on all machines. But check it:
      if (GetHostObj(objIdOnHost, &checkHost, &checkHostObjID))
      {
         // Interesting -- this had better point to the same thing. Note
         // that, due to timing issues, we might disagree about the owner,
         // but if it's implicitly proxied we had better agree about the
         // objID:
         if (objIdOnHost != checkHostObjID)
         {
            // Nope; they're inconsistent. This can happen legitimately
            // under rare circumstances -- for example, if a non-guaranteed
            // packet about a new object arrives before its creation message.
            // Best we can do is return null...
            ProxySpew(("ObjGetProxy got request for %d:%d, "
                       "which I don't know about. Returning OBJ_NULL.\n",
                       hostPlayer,
                       objIdOnHost));
            return OBJ_NULL;
         }
      }
      // If the object is local-only, no other player had better be talking
      // about it. If so, assume that it's an out-of-order packet.
      if ((hostPlayer != OBJ_NULL) &&
          (hostPlayer != PlayerObject()) &&
          mpObjSys->Exists(objIdOnHost) &&
          ObjLocalOnly(objIdOnHost))
      {
         ProxySpew(("ObjGetProxy got request for %d:%d, "
                    "which is local-only. Returning OBJ_NULL.\n",
                    hostPlayer,
                    objIdOnHost));
         return OBJ_NULL;
      }
      return (ObjID) objIdOnHost;
   }
}

STDMETHODIMP_(void) cObjectNetworking::ObjDeleteProxy(ObjID proxy)
{
   mbHandlingProxy = TRUE;
   mpObjSys->Destroy(proxy);
   mbHandlingProxy = FALSE;
}

// This should be called when we create a new object, to make sure we
// zero out any old proxy entries pointing to it:
void cObjectNetworking::ClearProxyEntry(ObjID obj)
{
   BOOL wasDeleted;
   if (mpDeletedProxyTable->Lookup(obj, &wasDeleted) && wasDeleted) {
      mpDeletedProxyTable->Delete(obj);
      // Kill any proxy entries pointing to this; there may be
      // multiples, if the object's been rehosted:
      cHashByHostObjID::cIter iter;
      for (iter = mpProxyTable->Iter();
           !iter.Done();
           iter.Next())
      {
         if (iter.Value() == obj) {
            // It's an entry pointing to this object:
            ProxySpew(("PROXY: Old proxy %d being reused.\n", obj));
            sHostObjID *key = iter.Key();
            mpProxyTable->Delete(key);
            delete key;
         }
      }
   }
}

////////////////////////////////////////////////////////////
//
// Object Ownership methods
//
// Note that these now *do* work even if we're not networking yet,
// so long as this is a networked game. This is to allow us to check
// object ownerships during postload. However, it is vital that this
// information not be used to actually send network message until after
// we have synch'ed, because we haven't necessarily resolved object
// remappings until then.
//

STDMETHODIMP_(BOOL) cObjectNetworking::ObjHostedHere(ObjID obj)
{
   if (obj == OBJ_NULL) {
      Warning(("ObjHostedHere called on a NULL object\n"));
      return FALSE;
   } else if (!mbNetworkGame) {
      // If we're not networking, then all hosted objects are hosted here:
      return (NetworkCategory(obj) != kLocalOnly);
   } else if (OBJ_IS_ABSTRACT(obj)) {
      // Abstract objects are, by definition, hosted by the default host
      return mpNetMan->AmDefaultHost();
   } else if (NetworkCategory(obj) == kLocalOnly) {
      // Local objects are, by definition, not hosted:
      return FALSE;
   } else if (!PlayerObjectExists()) {
      // The player hasn't been created yet, so all object presumptively
      // belong to the default host, unless we're in the middle of creating
      // it:
      if (obj == mObjBeingCreated)
         return TRUE;
      else
         return mpNetMan->AmDefaultHost();
   } else {
      // It's an ordinary object, under ordinary circumstances.
      // Who owns it?
      return (ObjHostPlayer(obj) == PlayerObject());
   }
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) cObjectNetworking::ObjIsProxy(ObjID obj)
{
   if (obj == OBJ_NULL) {
      Warning(("ObjIsProxy called on a NULL object\n"));
      return FALSE;
   } else if (!mbNetworkGame) {
      // If we're not networking, then there are no proxy objects
      return FALSE;
   } else if (OBJ_IS_ABSTRACT(obj)) {
      // Abstract objects are, by definition, hosted by the default host
      return !mpNetMan->AmDefaultHost();
   } else if (NetworkCategory(obj) == kLocalOnly) {
      // Local-only objects are, by definition, not hosted
      return FALSE;
   } else if (!PlayerObjectExists()) {
      // Player hasn't been created yet; all objects presumptively belong
      // to the default host, unless we're currently creating it:
      if (obj == mObjBeingCreated)
         return FALSE;
      else
         return !mpNetMan->AmDefaultHost();
   } else {
      // Ordinary object; who owns it?
      return (ObjHostPlayer(obj) != PlayerObject());
   }
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) cObjectNetworking::ObjLocalOnly(ObjID obj)
{
   if (obj == OBJ_NULL) {
      Warning(("ObjLocalOnly called on a NULL object\n"));
      return FALSE;
   } else if (OBJ_IS_ABSTRACT(obj)) {
      // Abstract objects are, by definition, hosted by the default host
      return FALSE;
   } else {
      return(NetworkCategory(obj) == kLocalOnly);
   }
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(ObjID) cObjectNetworking::ObjHostPlayer(ObjID obj)
{
   ObjID host;
   ObjID hostObjID;
   if (!mbNetworkGame) {
      // If we're not networking, then the object had better be
      // hosted here:
      return PlayerObject();
   } else if (OBJ_IS_ABSTRACT(obj)) {
      // Abstract objects are by definition hosted by the world host
      return mpNetMan->DefaultHost();
   } else if (GetHostObj(obj, &host, &hostObjID))
      return host;
   else if (NetworkCategory(obj) != kLocalOnly)
      // No table entry means it's hosted by defaulthost.
      return mpNetMan->DefaultHost();
   else
      return OBJ_NULL;
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(NetObjID) cObjectNetworking::ObjHostObjID(ObjID obj)
{
   ObjID host;
   ObjID hostObjID;
   if (!mbNetworkGame) {
      // If we're not networking, then it's the same objnum
      return obj;
   } else if (GetHostObj(obj, &host, &hostObjID))
      return hostObjID;
   else
      return obj;
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cObjectNetworking::ObjHostInfo(ObjID obj, 
                                                   ObjID *hostPlayer, 
                                                   NetObjID *objIdOnHost)
{
   ObjID tryObjIdOnHost;
   if (OBJ_IS_ABSTRACT(obj)) {
      *hostPlayer = mpNetMan->DefaultHost();
      *objIdOnHost = obj;
   } 
   else if (GetHostObj(obj, hostPlayer, &tryObjIdOnHost))
   {
      // Got it
      *objIdOnHost = tryObjIdOnHost;
      return;
   }
   else if (NetworkCategory(obj) == kLocalOnly)
   {
      *hostPlayer = OBJ_NULL;
      *objIdOnHost = obj;
   }
   else
   {
      if (mbNetworkGame) {
         *hostPlayer = mpNetMan->DefaultHost();
      } else {
         *hostPlayer = PlayerObject();
      }
      *objIdOnHost = obj;
   }
}

////////////////////////////////////////////////////////////
//
// OBJECT REMAPPING
//
// In a perfect world, we would be able to load files, and know that the
// object numbers loaded are the same as the ones we saved, and that the
// objIDs are the same from player to player. Unfortunately, this isn't
// a perfect world.
//
// In fact, objects often have to be remapped at load time. There are
// several causes, but they all boil down to the fact that objIDs can
// be usurped before a file gets loaded. We might be combining a briefcase
// from one level with another level's inherent objects; loading an early
// file might create some transient objects that trample on the ObjIDs of
// a later file; whatever. Regardless, the networking system has to cope
// with this sort of remapping.
//
// We keep track of each remapped object, as well as the file "number"
// that it was loaded from, and tell the other players about these
// remappings. When we're done with the loading, and pick up our messages
// again, we deal with any remappings from the other players, swizzling
// them through our own remapping table if needed, and come up with the
// actual mapping to put into the proxy table.
//
// Note that we explicitly assume that all of the players are loading the
// same files in the same order, so we can get away with just tracking
// and transmitting file "numbers". (Where the file number represents the
// load order since the last reset.) If this assumption proves false, this
// is all gonna break horribly, and we're going to have to actually tell
// each other file names. (Ick.)
//

//
// An object has been remapped at load time.
//
// Note that this assumes that it is *only* called at load time; the
// results are likely to be unpredictable if it gets called at any other
// time.
//
STDMETHODIMP_(void) cObjectNetworking::NotifyObjRemapped
   (ObjID newObjID,
    ObjID oldObjID)
{
   // Create an entry in the remap table for this object
   Assert_(mpRemapTable);
   Assert_(mpProxyRemapTable);

   // What file are we currently loading?
   int filenum = dbCurrentFilenum();

   ProxySpew(("REMAP: file %d; mapping %d->%d\n", 
              filenum, oldObjID, newObjID));

   // Record the remapping for the sake of proxies:
   sProxyFileObjID *proxyObjID = new sProxyFileObjID;
   proxyObjID->filenum = filenum;
   proxyObjID->newLocalObjID = newObjID;
   mpProxyRemapTable->Set(proxyObjID, oldObjID);

   // Record the remapping, in case this was an implicitly hosted
   // object:
   sFileObjID *fileObjID = new sFileObjID;
   fileObjID->filenum = filenum;
   fileObjID->owner = OBJ_NULL;
   fileObjID->oldHostObjID = oldObjID;
   mpRemapTable->Set(fileObjID, newObjID);

   // Store the data away:
   if (!NetSynchRemapMsgsDisabled()) {
      cRemapToSend *pPending =
         new cRemapToSend(filenum, oldObjID, newObjID);
      mRemapToSendList.Prepend(pPending);
#ifdef PLAYTEST
      // We're going to also kick out some late messages, just to check
      // the remappings:
      if (config_is_defined("net_check_remaps"))
      {
         pPending = new cRemapToSend(filenum, oldObjID, newObjID);
         mCheckRemapList.Prepend(pPending);
      }
#endif
   }
}

// Send all of the remappings that we have built up. We have to store them
// up, because we only want to send the ones that we are *hosting*, not
// proxies and not local objects. At the time of NotifyObjRemapped, we
// haven't loaded the properties yet, so we don't know.
void cObjectNetworking::SendRemappings()
{
   Assert_(mpNetMan->IsNetworkGame());

   cRemapToSend *pPending, *pNext;
   uint numRemaps = 0;

   // First pass: run through the list, winnow it down to the ones we
   // really want to send, and count those up:
   for (pPending = mRemapToSendList.GetFirst();
        pPending != NULL;
        pPending = pNext)
   {
      pNext = pPending->GetNext();

      // We can't use ObjHostedHere, since we're not networking yet:
      ObjID obj = pPending->m_newObjID;
      ObjID host;
      ObjID hostObjID;
      BOOL myObj;
      if (GetHostObj(obj, &host, &hostObjID)) {
         if (host == PlayerObject())
            myObj = TRUE;
         else
            myObj = FALSE;
      } else {
         if (mpNetMan->AmDefaultHost())
            myObj = TRUE;
         else
            myObj = FALSE;
      }
      if (myObj && (NetworkCategory(obj) != kLocalOnly))
      {
         numRemaps++;
      } else {
         mRemapToSendList.Remove(pPending);
         delete pPending;
      }
   }

   if (numRemaps > 0)
   {
      // Allocate the block to hold all of the remappings:
      int blockSize = numRemaps * sizeof(sSingleRemap);
      void *pBlock = malloc(blockSize);
      sSingleRemap *pMapping = (sSingleRemap *) pBlock;

      // Second pass: actually build up the remappings in the block:
      for (pPending = mRemapToSendList.GetFirst();
           pPending != NULL;
           pPending = pNext)
      {
         pNext = pPending->GetNext();

         pMapping->fileNum = pPending->m_fileNum;
         pMapping->oldObjID = pPending->m_oldObjID;
         pMapping->newObjID = pPending->m_newObjID;

         ProxySpew(("Sending Remapping:\n"
                    "   FileNum: %d\n"
                    "   OldObjID: %s\n"
                    "   NewObjID: %s\n",
                    pMapping->fileNum,
                    ObjWarnName(pMapping->oldObjID),
                    ObjWarnName(pMapping->newObjID)));

         pMapping++;
         mRemapToSendList.Remove(pPending);
         delete pPending;
      }

      // This message is defined up above; it lands in HandleRemap
      mpRemapMsg->Send(OBJ_NULL, 
                       mpNetMan->MyPlayerNum(), 
                       blockSize,
                       pBlock);

      free(pBlock);
   }

   ProxySpew(("Done sending object remappings.\n"));
}

//
// This should get called once we're actually up and networking. It will
// send a bunch of trivial messages, about each object that we have
// remapped. It is intended to exercise the code in cNetMsg that checks
// object consistency, and thereby ensure that all of our remappings are
// okay.
//
void cObjectNetworking::SendRemapChecks()
{
#ifdef PLAYTEST
   Assert_(mpNetMan->IsNetworkGame());

   cRemapToSend *pPending, *pNext;

   for (pPending = mCheckRemapList.GetFirst();
        pPending != NULL;
        pPending = pNext)
   {
      pNext = pPending->GetNext();

      ObjID obj = pPending->m_newObjID;
      if (NetworkCategory(obj) != kLocalOnly)
      {
         // This message is defined up above. We don't actually do anything
         // with it on the other side.
         mpCheckRemapMsg->Send(OBJ_NULL, obj);
      }
      mCheckRemapList.Remove(pPending);
      delete pPending;
   }
#endif
}

//
// Deal with a remapping that happened on another machine. We need to
// create a proxy entry for it. What makes this tricky is that we
// first have to check if *we* remapped the object, which can affect
// what the proxy needs to point to...
//
// Note that we don't actually create the proxy entry quite yet.
// Problem is that the "from" we get here is generally 0, because this
// message is being sent and received during metagame mode. So we have
// to record the player *number* (instead of his objID), and buffer
// things up until we actually have a real player ID, at which point
// we can stick it into the proxy table...
//
void cObjectNetworking::HandleARemap(uchar playerNum, ushort filenum,
                                     ObjID oldObjID, ObjID newObjID)
{
   ProxySpew(("Received Remapping:\n"
              "   FileNum: %d\n"
              "   OldObjID: %s\n"
              "   NewObjID: %s\n",
              filenum,
              ObjWarnName(oldObjID),
              ObjWarnName(newObjID)));

   // Store it away until we have the player's objID:
   cPendingRemap *pPending =
      new cPendingRemap(playerNum, filenum, oldObjID, newObjID);
   mPendingRemapList.Prepend(pPending);
}

//
// Get a bunch of remappings from another player, and break it down
// into the individual mappings.
//
void cObjectNetworking::HandleRemap(uchar playerNum,
                                    uint remapSize, sSingleRemap *remaps)
{
   sSingleRemap *pEnd = (sSingleRemap *) (((char *) remaps) + remapSize);
   while (remaps < pEnd)
   {
      HandleARemap(playerNum, remaps->fileNum, 
                   remaps->oldObjID, remaps->newObjID);
      remaps++;
   }
}

//
// Now we know the objID for this player, so check any of his remappings
// into the proxy table.
//
STDMETHODIMP_(void) cObjectNetworking::ResolveRemappings
   (ulong playerNum,
    ObjID playerID)
{
   cPendingRemap *pPending, *pNext;

   BOOL isDefaultHost = (playerID == mpNetMan->DefaultHost());

   for (pPending = mPendingRemapList.GetFirst();
        pPending != NULL;
        pPending = pNext)
   {
      pNext = pPending->GetNext();
      if (pPending->m_playerNum == playerNum) {
         // It's pending for this player
         // Have we remapped this thing ourselves?
         ObjID localObjID;
         BOOL gotLocalObjID = FALSE;
         sFileObjID fileObjID = {pPending->m_fileNum,
                                 playerID,
                                 pPending->m_oldObjID};
         if (mpRemapTable->Lookup(&fileObjID, &localObjID)) {
            gotLocalObjID = TRUE;
         } else if (isDefaultHost) {
            // For the default host, we have to also check against
            // implicitly hosted objects.
            fileObjID.owner = OBJ_NULL;
            if (mpRemapTable->Lookup(&fileObjID, &localObjID)) {
               // This entry means that either it was implicitly hosted,
               // *or* this is an unrelated object that I own, that happens
               // to have the same objID. We can't call ObjIsProxy yet,
               // because we're not officially networking, so we have to
               // resort to more primitive means: it's implicitly hosted
               // if it is hosted and does *not* have a proxy link. We
               // can assume that we're not the default host, since we're
               // currently processing msgs *from* the default host.
               ObjID dummyHost;
               ObjID dummyHostObjID;
               if ((NetworkCategory(localObjID) != kLocalOnly) &&
                   (!GetHostObj(localObjID, &dummyHost, &dummyHostObjID)))
               {
                  // It really is implicitly hosted.
                  gotLocalObjID = TRUE;
               } else {
                  ProxySpew(("I think object %d isn't a proxy.\n", localObjID));
               }
            }
         }

         if (!gotLocalObjID) {
            // We didn't remap it, so we haven't yet deproxified:
            localObjID = ObjGetProxy(playerID, pPending->m_oldObjID);
         }

         // Okay, now delete the old table entry, if there is one:
         sHostObjID oldHostObjID = {playerID, pPending->m_oldObjID};
         ObjID result;
         if (mpProxyTable->Lookup(&oldHostObjID, &result) &&
             (result == localObjID))
         {
            // Yep, this is an old table entry, which is now obsolete
            ProxySpew(("Deleting old mapping %d:%d -> %d\n",
                       playerID, pPending->m_oldObjID, result));
            sHostObjID *oldKey = mpProxyTable->GetKey(&oldHostObjID);
            mpProxyTable->Delete(&oldHostObjID);
            delete oldKey;
         }

         ProxySpew(("Remapping %d:%d to my %s\n",
                    playerID,
                    pPending->m_remappedObjID,
                    ObjEditName(localObjID)));
         ObjRegisterProxy(playerID, 
                          pPending->m_remappedObjID,
                          localObjID);
         mPendingRemapList.Remove(pPending);
         delete pPending;
      }
   }
}

////////////////////////////////////////////////////////////
//
// Object Networking Loop Client
//

// State record for use by the network loop client
typedef struct _StateRecord
{
   cObjectNetworking *pObjNet;
} StateRecord;

void cObjectNetworking::db_message(DispatchData *msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   if (!mpNetMan->IsNetworkGame())
      return;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         ClearTables();
         break;
      case kDatabaseLoad:
         break;
      case kDatabasePostLoad:
         RebuildProxyTable();
         SendRemappings();
         break;
      case kDatabaseSave:
         break;
   }
}

static eLoopMessageResult LGAPI _LoopFunc(void* data, 
                                          eLoopMessage msg, 
                                          tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   cObjectNetworking *This = state->pObjNet;
   LoopMsg info;
   info.raw = hdata;

   switch(msg)
   {
      case kMsgDatabase:
      {
         This->db_message(info.dispatch);
         break;
      }
      case kMsgEnd:
         delete state;
         break;   
   }
   return result;
}

// Factory function for our loop client.
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc,
                                        tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = new StateRecord;
   state->pObjNet = g_pObjNet;

   return CreateSimpleLoopClient(_LoopFunc,state,&ObjNetLoopClientDesc);
}

// The public loop client descriptor
// used by loopapp.c
sLoopClientDesc ObjNetLoopClientDesc =
{
   &LOOPID_ObjNet,
   "ObjNet Loop Client",              
   kPriorityNormal,              
   kMsgDatabase | kMsgsMode | kMsgEnd, // interests 

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      // No specific constraints at this point:
      {kNullConstraint}
   }
};
////////////////////////////////////////////////////////////

tResult LGAPI ObjectNetworkingCreate(void)
{
   IUnknown* pOuter = AppGetObj(IUnknown);
   IObjectNetworking* sys = new cObjectNetworking(pOuter);
   return (sys != NULL) ? NOERROR : E_FAIL;
}
