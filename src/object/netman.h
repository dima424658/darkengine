/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/netman.h,v 1.31 2000/01/29 13:23:47 adurant Exp $
#pragma once

#ifndef __NETMAN_H
#define __NETMAN_H

#include <comtools.h>
#include <propbase.h>  // for sPropertyObjIter
#include <nettypes.h>

F_DECLARE_INTERFACE(INetManager);

////////////////////////////////////////////////////////////
// NETWORKING COM INTERFACES
//

// Signature for the callback function registered for by systems that
// want to handle some of the messages coming in off the net.
typedef void (*tNetMessageParser)(const sNetMsg_Generic *pMsg, 
                                  ulong msgSize,
                                  ObjID fromPlayer,
                                  void *pClientData);

// Signature of a listener on the networking system. situation is whatever
// is happening right now; data is situation-dependent; pClientData is
// specified when the listener gets set up. If data is relevant, it
// should be a (tNetListenData *), as specified in netnotif.h.
typedef void (*tNetListenerCallback)(eNetListenMsgs situation,
                                     DWORD data,
                                     void *pClientData);

// Signature of a callback function for SynchFlush.
typedef BOOL (*tNetSynchFlushCallback)(void *pClientData);

// Flags to pass to RegisterMessageParser
typedef enum eNetHandlerFlags_ 
{ 
   // means that network statistics should be gathered assuming that
   // the second byte of the message is a subparser index:
   kNetSubparse = 1<<0,
   // means this message type should be allowed through even if we are
   // in a NonNetworkLevel. Use with great care:
   kNetAllowNonNetwork = 1<<1,
} _eNetHandlerFlags;
typedef ulong eNetHandlerFlags;

// The handle type returned from Listen()
typedef int tNetListenerHandle;

//////////////////////////////////////////////////////////////////
//
// FOR_ALL_NET_PLAYERS(INetManager *netman, ObjID obj)
//
// A macro for looping through network players (doesn't include
// PlayerObject()). Can't be used in C, except at the beginning of a
// block due to the declaration. When the loop ends, pPlayerObj is
// set to null.
//
// BE CAREFUL NOT TO PUT A ';' AFTER THE "FOR_ALL_NET_PLAYERS" LINE
// (i.e. the null loop body).
//
#define FOR_ALL_NET_PLAYERS(pNetMan, pPlayerObj) \
   sPropertyObjIter iter; \
   (pNetMan)->NetPlayerIterStart(&iter); \
   while ((pNetMan)->NetPlayerIterNext(&iter, (pPlayerObj)) || (*(pPlayerObj)=OBJ_NULL,FALSE))

// This version also includes the PlayerObject.
#define FOR_ALL_PLAYERS(pNetMan, pPlayerObj) \
   sPropertyObjIter iter; \
   (pNetMan)->NetPlayerIterStart(&iter); \
   for (*(pPlayerObj) = PlayerObject(); \
        *(pPlayerObj) != OBJ_NULL; \
        (pNetMan)->NetPlayerIterNext(&iter, (pPlayerObj)) ? 0 : *(pPlayerObj) = OBJ_NULL)

// The longest allowed player name
#define MAX_PLAYER_NAME_LEN 32

#undef INTERFACE
#define INTERFACE INetManager

DECLARE_INTERFACE_( INetManager, IUnknown )
{
   DECLARE_UNKNOWN_PURE();

   // Is this a multi-player game, ready for sending & receiving messages.
   STDMETHOD_(BOOL, Networking)(THIS) PURE;

   // Is this a multi-player game, not necessarily ready for sending &
   // receiving messages.
   STDMETHOD_(BOOL, IsNetworkGame)(THIS) PURE;

   // Current number of players, including yourself.
   STDMETHOD_(ulong, NumPlayers)(THIS) PURE;

   // PlayerNum is a number between 1 & NumPlayers that is unique
   // for each player (1 == defaulthost). Note that the PlayerNum
   // has one very important quality: it is consistent between loads.
   // Therefore, it can be used with some confidence in properties.
   STDMETHOD_(ulong, MyPlayerNum)(THIS) PURE;
   STDMETHOD_(ulong, ObjToPlayerNum)(THIS_ ObjID player) PURE;
   STDMETHOD_(ObjID, PlayerNumToObj)(THIS_ ulong player) PURE;

   // Set the player's name on the network. Should be called before
   // Host() or Join().
   STDMETHOD_(void, SetPlayerName)(THIS_ const char *pName) PURE;
   // Get the name of a player, given an ObjID. If the ObjID is OBJ_NULL,
   // return the name of this player.
   STDMETHOD_(const char *, GetPlayerName)(THIS_ ObjID player) PURE;
   // Get the net address of the specified player, as a string. If OBJ_NULL
   // is passed in, will return the net address of the current player.
   STDMETHOD_(const char *, GetPlayerAddress)(THIS_ ObjID player) PURE;

   // Prepare to host a networked game. pMedia gives the name of the
   // media type to connect with; pSession gives the name of the session
   // to establish. Both can be NULL. Returns TRUE iff the session was
   // successfully established.
   STDMETHOD_(BOOL, Host)(THIS_ const char *pMedia, const char *pSession)
      PURE;

   // Join an existing networked game. Same params as for Host(), plus
   // pAddress should be the IP address or DNS name of the host for this
   // game. pAddress is mandatory. Returns TRUE iff we were able to join
   // the game.
   STDMETHOD_(BOOL, Join)(THIS_ 
                          const char *pMedia,
                          const char *pSession,
                          const char *pAddress) PURE;

   // Get/Set the timeout for this session. This is sometimes worth
   // increasing, if a glitchy connection is sometimes causing networking
   // to be lost.
   STDMETHOD_(ulong, GetTimeout)(THIS) PURE;
   STDMETHOD_(void, SetTimeout)(THIS_ ulong timeout) PURE;

   // Temporarily suspend outgoing messages, except MetagameBroadcasts.
   // Allows stacking.
   STDMETHOD_(void, SuspendMessaging)(THIS) PURE;
   // Is networking suspended? This method returns the stack count of
   // Suspends. If it returns 0, messaging is not suspended.
   STDMETHOD_(int, Suspended)(THIS) PURE;
   // Resume temporarily suspended outgoing messages. 
   STDMETHOD_(void, ResumeMessaging)(THIS) PURE;

   // Send this message to 'player'
   // Note that you can *always* Send to yourself, even if networking is
   // not currently turned on. This is to allow consistent host/client
   // separation, without having to worry about whether networking is
   // on.
   STDMETHOD_(void, Send)(THIS_
                          ObjID player,
                          void *msg,
                          ulong size,
                          BOOL guaranteed) PURE;

   // Send this message to all other players.
   STDMETHOD_(void, Broadcast)(THIS_
                               void *msg,
                               ulong size,
                               BOOL guaranteed) PURE;

   // Send this message to all other player, even if we are not actually
   // in game mode. Guaranteed is assumed for metagame messages. If
   // always is TRUE, then this will go through even if we're in
   // NonNetworkLevel. In that case, the message should have been
   // created with the AllowNonNetwork flag set.
   STDMETHOD_(void, MetagameBroadcast)(THIS_
                                       void *msg,
                                       ulong size,
                                       BOOL always DEFAULT_TO(FALSE))
      PURE;

   // The host that hosts most in-world objects (i.e. not in someones
   // inventory, nor an AI).  This only works after all the
   // initialization process is complete (Networking() is TRUE).
   STDMETHOD_(ObjID, DefaultHost)(THIS) PURE;
   // Is this machine the default host. Unlike DefaultHost(), this
   // works very early in the intialization process (after
   // netman::Init()).
   STDMETHOD_(BOOL, AmDefaultHost)(THIS) PURE;
   // Get the name of the default host. This also works very early, pretty
   // much right after we have connected. The host's name can be used as
   // a rough-and-ready ID mechanism. Guaranteed to never return NULL.
   STDMETHOD_(const char *, GetDefaultHostName)(THIS) PURE;

   // Game components can register a message parser for handling
   // incoming messages.
   STDMETHOD_(tNetMsgHandlerID, RegisterMessageParser)
      (THIS_ 
       tNetMessageParser parser, 
       const char *moduleName,
       eNetHandlerFlags flags,
       void *pClientData) PURE;

   ///////////////////////
   //
   // Routines for iterating over the network players (doesn't include
   // the local player) (The property that this is based on is not
   // public).  No IterStop is necessary.
   //
   STDMETHOD_(void, NetPlayerIterStart)
      (THIS_ sPropertyObjIter* iter) CONSTFUNC PURE; 
   STDMETHOD_(BOOL, NetPlayerIterNext)
      (THIS_ sPropertyObjIter* iter,ObjID* next) CONSTFUNC PURE; 

   // Register a listener, which will be called whenever the specified
   // event happens. The returned handle can be passed to Unlisten().
   STDMETHOD_(tNetListenerHandle, Listen) 
      (THIS_ 
       tNetListenerCallback callback,
       eNetListenMsgs interests,
       void *pClientData) PURE;

   // Convert to or from a global representation of an object, for
   // when the object is not known to be hosted by either the sender
   // or the receiver of the message.
   STDMETHOD_(sGlobalObjID, ToGlobalObjID)(THIS_ ObjID obj) PURE;
   STDMETHOD_(ObjID, FromGlobalObjID)(THIS_ sGlobalObjID *otherObj) PURE;

   // Get the ObjID of the player who sent the message currently being
   // processed. This is available for subsystems that need to respond to
   // a message, but are separated from the playerID by many layers of
   // code. (In other words, it's a back-compatibility hack.)
   // Returns PlayerObject() if we're not responding to a network message,
   // including if we're not Networking().
   STDMETHOD_(ObjID, OriginatingPlayer)(THIS) PURE;

   // Begin the process of synchronizing with other network players.
   // This should get called before any real networking, whenever we
   // reset the object database.
   STDMETHOD_(void, StartSynch)(THIS) PURE;

   // Declare that we are entering a level that should not be at all
   // networked. Networking will be *entirely* suppressed until we get
   // a NormalLevel(). Currently, this can only be switched at level
   // transitions. Timing is critical: these calls should come between
   // resetting the database and loading the new level, because even
   // the beginnings of level loading can include important network
   // messages.
   STDMETHOD_(void, NonNetworkLevel)(THIS) PURE;
   STDMETHOD_(void, NormalLevel)(THIS) PURE;

   // Flush all the buffers, and wait for all the other machines to
   // do the same. This should be called after sending some message
   // around that will cause all of the machines to do the same thing.
   // That is, it assumes that the other machines are going into
   // SynchFlush() at about the same time. Normally returns TRUE.
   //
   // IMPORTANT: THIS METHOD BLOCKS. It will wait until all of the
   // machines are ready.
   //
   // Before calling SynchFlush(), you must call PreFlush() to set it up.
   // This is necessary in order to avoid possible timing snafus.
   // PreFlush() MUST be called immediately after receiving any
   // network message that will lead to the SynchFlush(); it preps
   // us to begin counting who has done their flushes. It is safe to
   // call PreFlush() spuriously, if there is any worry about that.
   //
   // If a callback is specified, it will be called repeatedly while
   // we sit and spin. It should normally return TRUE; if it ever
   // returns FALSE, SynchFlush() will fail and return FALSE. This can
   // be used to implement a timeout mechanism.
   STDMETHOD_(void, PreFlush)(THIS) PURE;
   STDMETHOD_(BOOL, SynchFlush)(THIS_
                                tNetSynchFlushCallback callback,
                                void *pClientData) PURE;

   // Get the name of a player, given a player number. This version
   // of GetPlayerName works relatively early in metagame, any time
   // after you have been notified with a kNetMsgPlayerInfo message.
   STDMETHOD_(const char *, GetPlayerNameByNum)(THIS_ int playerNum) PURE;
   // Get the net address of the specified player, as a string.
   STDMETHOD_(const char *, GetPlayerAddressByNum)(THIS_ int playerNum) PURE;

   // End this networking session. Disconnects all clients if you are
   // the host.
   STDMETHOD_(void, Leave)(THIS) PURE;

   // Stop worrying about this particular listener.
   STDMETHOD_(void, Unlisten)(THIS_ tNetListenerHandle handle) PURE;

   // Establish a limit to the number of players allowed in the game.
   // Any players above that limited will be rejected. Note that
   // rejected players will still be able to successfully Join(), but
   // they will get a kNetMsgRejected instead of kNetMsgHi. There is
   // a non-crazy default, so this is optional, but should be set
   // appropriately for the application.
   STDMETHOD_(void, SetMaxPlayers)(THIS_ int maxPlayers) PURE;
};

#define INetMan_Networking(p)              COMCall0(p, Networking)

//
// Factory Function 
//
EXTERN void NetManagerCreate(void);

#endif  // __NETMAN_H


