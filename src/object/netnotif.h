/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/netnotif.h,v 1.6 2000/01/29 13:23:49 adurant Exp $
//
// Notifications for the networking system
//
// These are the flags and data types that should be passed into
// INetManager::Listen(), and that are received from the listeners.
//
#pragma once

#ifndef __NETNOTIF_H
#define __NETNOTIF_H

// The various sorts of messages that a listener can register interest in:
typedef enum eNetListenMsgs_
{
   // A player has connected:
   kNetMsgPlayerConnect = 1 << 0,

   // A player connection has been lost. Player ObjID is in data field:
   kNetMsgPlayerLost = 1 << 1,

   // A player has joined (that is, we now have their full game info).
   // Player ObjID is in data field:
   kNetMsgPlayerJoin = 1 << 2,

   // We are currently synchronizing game info, just after all players
   // have joined:
   kNetMsgSynchronize = 1 << 3,

   // We are now fully networking, with all players readied. This has
   // exactly the semantics of the Networking() method, until the next
   // Reset. Clients should assume that Networking is off, until
   // notified that it is on:
   kNetMsgNetworking = 1 << 4,

   // Networking has been reset; players are still connected, but will
   // have to rejoin. Effectively, Networking() is no longer true:
   kNetMsgReset = 1 << 5,

   // Only sent for clients; indicates that the host has acknowledged
   // our existence. Player number is in data field:
   kNetMsgHi = 1 << 6,

   // Network connection has been lost, for whatever reason. Sadness.
   kNetMsgNetworkLost = 1 << 7,

   // Only sent for clients; we tried to join, and the host has told
   // us to bugger off. Reason for the rejection is in the data field,
   // and is from eNetListenRejectReasons_.
   kNetMsgRejected = 1 << 8,

   // We have begun to network. This basically indicates that this is
   // now a networked game, until and unless a NetworkLost message is sent.
   kNetMsgNetworkGame = 1 << 9,

   // We've gotten info from a player; listeners should update the player's
   // name and/or address info. Player number is in data field:
   kNetMsgPlayerInfo = 1 << 10,
} _eNetListenMsgs;

// The reason why we have been rejected:
typedef enum eNetListenRejectReasons_
{
   // Miscellaneous failure:
   kNetRejectMisc = 1 << 0,

   // The game has already started:
   kNetRejectStarted = 1 << 1,

   // There is already a full slate of players:
   kNetRejectTooMany = 1 << 2,

   // You don't have the same version of the game as the host:
   kNetRejectVersion = 1 << 3,
} _eNetListenRejectReasons;

#endif // !__NETNOTIF_H
