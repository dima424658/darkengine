/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/iobjnet_.h,v 1.20 2000/01/29 13:41:16 adurant Exp $
//
// Internals for object networking.
//
#pragma once

#ifndef __IOBJNET__H
#define __IOBJNET__H

#include <iobjsys.h>
#include <iobjnet.h>
#include <aggmemb.h>

#include <hashpp.h>
#include <dlist.h>

#include <property.h>
#include <propface.h>

#include <dispatch.h>

#include <nettypes.h>

#include <netmsg.h>

F_DECLARE_INTERFACE(INetManager);

// The kinds of network messages the object system can generate and
// receive:
typedef enum eObjSubparser {
   kObjSysMsg_BeginCreate,
   kObjSysMsg_EndCreate,
   kObjSysMsg_DestroyObj,
   kObjSysMsg_NewOwner,
   kObjSysMsg_Remap,
} eObjSubparser;
// The actual type for that enumeration:
typedef uchar tObjSubparser;

// The "generic" object message:
struct sNetMsg_Obj {
   tNetMsgHandlerID handlerID;  // Should always point to the objsys
   tObjSubparser subparser;
};

// The obj-in-file structure. This is used as the basis of the object
// remapping table.
struct sFileObjID {
   int filenum;
   ObjID owner;
   ObjID oldHostObjID;
};

// The proxy-in-file structure. This is the basis of the proxy
// remapping backlink table.
struct sProxyFileObjID {
   int filenum;
   ObjID newLocalObjID;
};

// The list of remappings waiting to be inserted into the proxy table,
// once we know the actual player ObjID:
class cPendingRemap;
typedef cDList<cPendingRemap, 1> cPendingRemapList;
typedef cDListNode<cPendingRemap, 1> cPendingRemapNode;
class cPendingRemap : public cPendingRemapNode
{
public:
   cPendingRemap(ulong playerNum,
                 ushort fileNum,
                 ObjID oldObjID,
                 ObjID remappedObjID)
   {
      m_playerNum = playerNum;
      m_fileNum = fileNum;
      m_oldObjID = oldObjID;
      m_remappedObjID = remappedObjID;
   }

   ulong m_playerNum;
   ushort m_fileNum;
   ObjID m_oldObjID;
   ObjID m_remappedObjID;
};

// The list of remappings waiting to be sent to the other players, once
// we're sure that they're public knowledge and we own them:
class cRemapToSend;
typedef cDList<cRemapToSend, 1> cRemapToSendList;
typedef cDListNode<cRemapToSend, 1> cRemapToSendNode;
class cRemapToSend : public cRemapToSendNode
{
public:
   cRemapToSend(int fileNum, ObjID oldObjID, ObjID newObjID)
   {
      m_fileNum = fileNum;
      m_oldObjID = oldObjID;
      m_newObjID = newObjID;
   }

   int m_fileNum;
   ObjID m_oldObjID;
   ObjID m_newObjID;
};

//
// The single-remapping structure, used in the network message
//
struct sSingleRemap {
   ushort fileNum;
   ObjID oldObjID;
   ObjID newObjID;
};

//
// OBJECT SYSTEM IMPLEMENTATION CLASS
// 

class cObjectNetworking : public IObjectNetworking
{
   DECLARE_AGGREGATION(cObjectNetworking);

public:
   cObjectNetworking(IUnknown* pOuter);
   virtual ~cObjectNetworking();

   //
   // IObjectNetworking methods
   //
   STDMETHOD_(void, ClearTables)();
   STDMETHOD_(void, ObjRegisterProxy)(ObjID hostPlayer, NetObjID objIdOnHost, ObjID proxy);
   STDMETHOD_(ObjID,ObjGetProxy)(ObjID hostPlayer, NetObjID objIdOnHost);
   STDMETHOD_(void, ObjDeleteProxy)(ObjID proxy);
   STDMETHOD_(BOOL,ObjIsProxy)(ObjID obj);
   STDMETHOD_(BOOL,ObjHostedHere)(ObjID obj);
   STDMETHOD_(BOOL,ObjLocalOnly)(ObjID obj);
   STDMETHOD_(NetObjID,ObjHostObjID)(ObjID obj);
   STDMETHOD_(ObjID,ObjHostPlayer)(ObjID obj);
   STDMETHOD_(void,ObjHostInfo)(ObjID obj, ObjID *hostPlayer, NetObjID *objIdOnHost);
   STDMETHOD_(void,ObjTakeOver)(ObjID obj);
   STDMETHOD_(void,ObjGiveTo)(ObjID obj, ObjID player, ObjID newObjID);
   STDMETHOD_(void,ObjGiveWithoutObjID)(ObjID obj, ObjID player);

   STDMETHOD_(void, StartBeginCreate)(ObjID exemplar, ObjID obj);
   STDMETHOD_(void, FinishBeginCreate)(ObjID obj);
   STDMETHOD_(void, StartEndCreate)(ObjID obj);
   STDMETHOD_(void, FinishEndCreate)(ObjID obj);
   STDMETHOD_(void, StartDestroy)(ObjID obj);
   STDMETHOD_(void, FinishDestroy)(ObjID obj);
   STDMETHOD_(BOOL, HandlingProxy)();
   STDMETHOD_(void, NotifyObjRemapped)(ObjID newObjID, ObjID oldObjID);
   STDMETHOD_(void, ResolveRemappings)(ulong playerNum, ObjID playerID);
   STDMETHOD_(void, RequestDestroy)(ObjID obj);

   // These need to be public solely because they are called from a callback:
   void db_message(DispatchData *msg);
   void ObjSysListener(ObjID obj, eObjNotifyMsg msg);

   // Message handlers:
   void HandleBeginCreate(ObjID arch, ObjID obj, ObjID from);
   void HandleEndCreate(ObjID obj);
   void HandleDestroy(ObjID obj);
   void HandleNewOwner(ObjID proxy, ObjID newHostObjID, 
                       ObjID newOwner, int serial);
   void HandleRemap(uchar playerNum, uint remapSize, sSingleRemap *remaps);

protected: 

   //
   // Member methods
   //

   // Message generators:
   void BroadcastNewOwner(ObjID obj, ObjID newHost, ObjID newObjID);

   // Random handlers:
   void SendRemappings();
   void SendRemapChecks();
   void RebuildProxyTable();
   void ClearProxyEntry(ObjID obj);
   void SetProxyTableEntry(ObjID hostPlayer, 
                           NetObjID objIdOnHost, 
                           ObjID proxy);
   static void NetworkListener(eNetListenMsgs situation,
                               DWORD,
                               void *pClientData);
   void HandleARemap(uchar playerNum, ushort filenum, 
                     ObjID oldObjID, ObjID newObjID);

   //
   // Aggregate Protocol
   //
   HRESULT Init();
   HRESULT End();

   //
   // Member variables
   //

   // The other major related systems:
   INetManager* mpNetMan;
   IObjectSystem *mpObjSys;

   // TRUE iff we're in the middle of one of the Big Operations on an object:
   BOOL mbHandlingProxy;
   // The object that we are in process of creating a proxy for:
   ObjID mObjBeingProxied;
   ObjID mRealObjOwner;
   // TRUE iff networking is currently enabled:
   BOOL mbNetworking;
   // TRUE iff this is a networked game at all:
   BOOL mbNetworkGame;
   // The current depth of object creation; if this is above 1, then the
   // object should be local-only:
   int mCreationDepth;
   // The local-only objects that we are in the middle of creating. We
   // increment this in BeginCreate and decrement in EndCreate. Its sole
   // purpose is to deal with the rare case of an object that gets deleted
   // before EndCreate: we have to know whether or not it was local-only,
   // so we know how to react.
   int mLocalOnlies;
   // In the specific case where we don't have a player yet, we can't know
   // whether objects are local or remote. So we need to track creations
   // internally:
   ObjID mObjBeingCreated;

   //
   // The proxy-object table, which maps from remote objIDs to local ones.
   // NOTE: An entry is kept that maps a {host, objId} to a proxy objId, for
   // EVERY host that has ever owned this proxy.  That way old messages can
   // still understand objects are referred to, even if the host is old.
   typedef cGenHashFunc<sHostObjID> cHostObjIDFunc;
   typedef cHashTable<sHostObjID *, ObjID, cHostObjIDFunc> cHashByHostObjID;
   cHashByHostObjID *mpProxyTable;

   //
   // The object-remapping table, which maps from a remapped objects
   // real host ObjID to the new proxy ObjID. We use this in the remapping
   // process.
   typedef cGenHashFunc<sFileObjID> cFileObjIDFunc;
   typedef cHashTable<sFileObjID *, ObjID, cFileObjIDFunc> cHashByFileObjID;
   cHashByFileObjID *mpRemapTable;

   //
   // The object-remapping-backlink table, which records newly remapped
   // objects. We check those against the proxy table as we rebuild it,
   // to figure out if these were proxy objects, that we will need to
   // cope with.
   typedef cGenHashFunc<sProxyFileObjID> cProxyFileObjIDFunc;
   typedef cHashTable<sProxyFileObjID *, ObjID, cProxyFileObjIDFunc> cProxyRemapTable;
   cProxyRemapTable *mpProxyRemapTable;

   //
   // The deleted-objects table, which tracks proxies that have been
   // deleted but not yet removed. We intentionally put some delay into
   // our deletion of proxies, because messages can wander in late after
   // the proxy has been deleted. But we need to keep track of these,
   // because if the objnum is reused, we *must* delete all refs to the
   // proxy.
   typedef cScalarHashFunctions<ObjID> cObjHashFns;
   typedef cHashTable<ObjID, BOOL, cObjHashFns> cProxyListHash;
   cProxyListHash *mpDeletedProxyTable;

   //
   // The list of remote remapping which are waiting to be entered into
   // the proxy table. This is needed because we don't have the remote
   // players' ObjIDs when we get the remappings...
   cPendingRemapList mPendingRemapList;

   //
   // The list of remappings which are waiting to be sent to the other
   // players, after we're finished with our own loading:
   cRemapToSendList mRemapToSendList;

   //
   // During playtest builds, we kick out messages about each remapped
   // object, solely to exercise the object-consistency checks.
   cRemapToSendList mCheckRemapList;

   //
   // Modern-style messages to send.
   //
   cNetMsg *mpRequestDestroyMsg;
   cNetMsg *mpCreateObjPosMsg;
   cNetMsg *mpBeginCreateMsg;
   cNetMsg *mpEndCreateMsg;
   cNetMsg *mpDestroyMsg;
   cNetMsg *mpNewOwnerMsg;
   cNetMsg *mpRemapMsg;
   cNetMsg *mpCheckRemapMsg;
   cNetMsg *mpGiveObjectMsg;
};

#endif // __IOBJNET__H
