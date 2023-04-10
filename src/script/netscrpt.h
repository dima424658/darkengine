/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/netscrpt.h,v 1.15 2000/01/29 13:39:54 adurant Exp $
//
// Networking script services
//
// This subsystem is responsible for making it easy for a script to say,
// "replicate me on all the other machines". This mechanism is currently
// quite simplistic -- it is really just intended for replicating basic
// top-level messages.
//
#pragma once

#ifndef __NETSCRPT_H
#define __NETSCRPT_H

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Networking support functions
//

DECLARE_SCRIPT_SERVICE(Networking, 0x225)
{
   // Send this message to the specified object on all other machines.
   // Does nothing if networking isn't running.
   // @TBD: When the underlying networking code supports optional params
   // properly, beef this up to handle up to three params. Also, put in
   // a better flags field, so the caller can specify whether data fields
   // are integers or ObjIDs.
   STDMETHOD(Broadcast)(const object ref obj, 
                        const char *msg,
                        BOOL sendFromProxy = FALSE,
                        const cMultiParm ref data = NULL) PURE;

   // Send the specified message to the specified object on just the
   // specified player's machine. Should only be called by the machine
   // that owns the object, and the object should not be local-only.
   //
   // If the specified player is this machine's player, just does a
   // little short-circuit and sends the message. This allows some
   // code to be simpler. This code should always work, even if other
   // networking does not. (In other words, this can be used to send
   // to the real machine, not just the proxy.)
   //
   // @BRUTAL HACK: note that the data, if supplied, is treated as an
   // objID, *not* an int. Please, God, give us time to make cMultiParm
   // smarter...
   STDMETHOD(SendToProxy)(const object ref toPlayer, 
                          const object ref obj,
                          const char *msg,
                          const cMultiParm ref data = NULL) PURE;

   // Take control of the specified object. Does nothing if networking
   // isn't running. This should be used with care, and only if you
   // know what you're doing -- it should not be used casually.
   STDMETHOD(TakeOver)(const object ref obj) PURE;

   // Give an object to another player. This results in the object being
   // in a somewhat strange intermediate state for a second or so, while
   // it is "in transit". Again, use with care.
   STDMETHOD(GiveTo)(const object ref obj, const object ref toPlayer) PURE;

   // Returns TRUE iff the specified object is *any* player, including
   // one on some other machine.
   STDMETHOD_(BOOL, IsPlayer)(const object ref obj) PURE;

   // Returns TRUE iff this is a multiplayer game. Running just a
   // multiplayer host, without any clients, still counts as multiplayer.
   STDMETHOD_(BOOL, IsMultiplayer)() PURE;

   // Works the same as SetOneShotTimer, but sends the timed message to
   // the local object, even if it's a proxy. Occasionally useful for
   // objects with strange, complex networking.
   STDMETHOD_(timer_handle, SetProxyOneShotTimer)
      (const object ref toObj,
       const char *msg,
       float time,
       const cMultiParm & data = NULL_PARM) PURE;

   // Methods to iterate over the players. Note that these are *not*
   // re-entrant. FirstPlayer always returns an ObjID, since there is
   // always at least one player. NextPlayer will return ObjIDs until
   // they have all been given, at which point it will return OBJ_NULL.
   // There is no need to clean up if you abort the iteration early.
   STDMETHOD_(object, FirstPlayer)() PURE;
   STDMETHOD_(object, NextPlayer)() PURE;

   // Methods to temporarily suppress and re-enable networking. These do
   // stack, but be ultra-careful to make sure that it is not suppressed
   // for long. In general, use this only if you know what you're doing;
   // it can easily have side-effects.
   STDMETHOD(Suspend)() PURE;
   STDMETHOD(Resume)() PURE;

   // Methods to test object ownership. Most of the time, if you're in a
   // script, "self" is HostedHere, since most scripts are run on an
   // object's host. Exceptions will occur iff the object is LocalOnly,
   // in which case the other players don't know anything about this obj,
   // or if the message is a type that gets sent to proxies as well as
   // the host.
   STDMETHOD_(BOOL, HostedHere)(const object ref obj) PURE;
   STDMETHOD_(BOOL, IsProxy)(const object ref obj) PURE;
   STDMETHOD_(BOOL, LocalOnly)(const object ref obj) PURE;

   // Are we actually currently networking? This is different from
   // IsMultiplayer(), which just indicates whether this is a multiplayer
   // game; this method indicates whether we're really up and running.
   // In particular, object creations and suchlike really mustn't happen
   // when we're not networking...
   STDMETHOD_(BOOL, IsNetworking)() PURE;

   // Returns the player who owns the given object. If the object is local,
   // then that will be this player.
   STDMETHOD_(object, Owner)(const object ref obj) PURE;
};

////////////////////////////////////////////////////////////////////////////


#endif /* !__NETSCRPT_H */
