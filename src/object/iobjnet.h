/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/iobjnet.h,v 1.12 2000/01/29 13:23:07 adurant Exp $
//
// Object Networking
//
// This system serves as the interface between the Object System and the
// Network Manager. It deals with mapping objects between different machines,
// and keeping track of which machine "owns" which objects.
//
#pragma once

#ifndef __IOBJNET_H
#define __IOBJNET_H

#include <iobjsys.h>
#include <comtools.h>
#include <nettypes.h>

F_DECLARE_INTERFACE(IObjectNetworking);

#undef INTERFACE
#define INTERFACE IObjectNetworking
DECLARE_INTERFACE_(IObjectNetworking,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   ////////
   //
   // General-interest methods
   //

   //
   // Clear out all of the mapping tables. Generally gets called at
   // database reset time.
   //
   STDMETHOD_(void, ClearTables)(THIS) PURE;

   //
   // Establish the mapping from a remote ObjID to the local one. External
   // systems shouldn't usually need to call this, except in ususual
   // circumstances (eg, when the remote and local objects have different
   // archetypes).
   //
   STDMETHOD_(void, ObjRegisterProxy)(THIS_ 
                                      ObjID hostPlayer, 
                                      NetObjID objIdOnHost, 
                                      ObjID proxy) PURE;

   //
   // Given the host of an object and the ID on that host, returns the proxy objid that 
   // can be used on this machine.  
   //
   STDMETHOD_(ObjID,ObjGetProxy)(THIS_ 
                                 ObjID hostPlayer, 
                                 NetObjID objIdOnHost) PURE;

   //
   // Delete the proxy. Note that this is quite different from what happens
   // when you call ObjSys::Delete() on a proxy, which will ask the owning
   // machine to delete the object. This, instead, deletes only the local
   // proxy.
   // Use only if you know what you're doing!
   //
   STDMETHOD_(void, ObjDeleteProxy)(THIS_ ObjID proxy) PURE;

   //
   // Is this object a proxy for an object owned on another machine?
   // (False for kLocalOnly objects).
   //
   STDMETHOD_(BOOL,ObjIsProxy)(THIS_ ObjID obj) PURE;

   //
   // Is this object hosted on this machine (changes get broadcast).
   // (Also False for kLocalOnly objects).
   //
   STDMETHOD_(BOOL,ObjHostedHere)(THIS_ ObjID obj) PURE;

   //
   // Is this object local to this machine only, and unknown elsewhere?
   //
   STDMETHOD_(BOOL,ObjLocalOnly)(THIS_ ObjID obj) PURE;

   //
   // Get the player that hosts this obj.  HostPlayer will be OBJ_NULL for kLocalOnly
   // objects, and PlayerObj() for global objects that are hosted here.
   STDMETHOD_(ObjID,ObjHostPlayer)(THIS_ ObjID obj) PURE;

   //
   // Get the ObjID of obj on obj's host player's machine.
   STDMETHOD_(NetObjID,ObjHostObjID)(THIS_ ObjID obj) PURE;

   //
   // Get both the Host player and the ID of 'obj' on that player's machine.
   // (Faster than getting one at a time).
   STDMETHOD_(void,ObjHostInfo)(THIS_ 
                                ObjID obj, 
                                ObjID *hostPlayer,
                                NetObjID *objIdOnHost) PURE;

   //
   // Object handoff method. This method tells the object system to
   // take over responsibility for the specified object.
   STDMETHOD_(void,ObjTakeOver)(THIS_ ObjID obj) PURE;
   // This method gives the specified object to the specified player.
   // Requires that you know the proxy on the other side, so this needs
   // some setup. Generally only used by the default host.
   STDMETHOD_(void,ObjGiveTo)(THIS_ 
                              ObjID obj, 
                              ObjID newOwner, 
                              ObjID newObjID) PURE;
   // This version gives the specified object to the specified player,
   // and does *not* require you to know the proxy on the other side.
   // It works in a more phased way. As far as this machine is concerned,
   // the object has been handed off, and messages will be forwarded
   // accordingly. However, ownership will not be formally transferred
   // until the recipient gets the message, takes the object over, and
   // announces that to everyone else.
   //
   // While the object is in the transitional state, ObjHostObjID() will
   // return OBJ_NULL. If you need to send messages about the object,
   // therefore, you should use GIDs with the old ownership information.
   // (That is, this player and the local proxy.)
   STDMETHOD_(void,ObjGiveWithoutObjID)(THIS_
                                        ObjID obj,
                                        ObjID newOwner) PURE;

   ////////
   //
   // ObjSys methods
   //
   // These methods are mainly here for use by the Object System itself.
   //
   STDMETHOD_(void, StartBeginCreate)(THIS_ 
                                      ObjID exemplar, 
                                      ObjID obj) PURE;
   STDMETHOD_(void, FinishBeginCreate)(THIS_ ObjID obj) PURE;
   STDMETHOD_(void, StartEndCreate)(THIS_ ObjID obj) PURE;
   STDMETHOD_(void, FinishEndCreate)(THIS_ ObjID obj) PURE;
   STDMETHOD_(void, StartDestroy)(THIS_ ObjID obj) PURE;
   STDMETHOD_(void, FinishDestroy)(THIS_ ObjID obj) PURE;
   STDMETHOD_(BOOL, HandlingProxy)(THIS) PURE;
   STDMETHOD_(void, NotifyObjRemapped)(THIS_ 
                                       ObjID newObjID, 
                                       ObjID oldObjID) PURE;
   STDMETHOD_(void, ResolveRemappings)(THIS_
                                       ulong playerNum,
                                       ObjID playerID) PURE;
   STDMETHOD_(void, RequestDestroy)(THIS_ ObjID obj) PURE;
};

#undef INTERFACE 

EXTERN tResult LGAPI ObjectNetworkingCreate(void);

//////////
//
// C-style API, as needed
#define IObjNet_ObjHostedHere(p, a)     COMCall1(p, ObjHostedHere, a)
#define IObjNet_ObjIsProxy(p, a)        COMCall1(p, ObjIsProxy, a)

#endif // __IOBJNET_H
