/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objproxy.h,v 1.2 2000/01/29 13:23:57 adurant Exp $
//
// Proxy object manager
//
// In a networked environment, each machine "owns" some of the objects in
// the game. All of the other machines maintain "proxies" for those objects:
// simplified versions of the objects, which have the same archetype and
// position information, but which aren't as fully simulated as the real
// object.
//
// The proxy manager keeps track of hash tables that map between the real
// host objIDs to the local proxies. Conceptually, it is closely related
// to objsys.
//
#pragma once

#ifndef _OBJPROXY_H
#define _OBJPROXY_H

#include <hashpp.h>
#include <netprops.h> // for sHostObjID

//////////

class cProxyManager
{
public:
   cProxyManager();
   ~cProxyManager();

   void RegisterProxy(ObjID host, NetObjID netObjID, ObjID proxyID);
   ObjID GetProxy(ObjID host, NetObjID netObjID);
   void DeleteProxy(ObjID host, NetObjID netObjID);

private:
   typedef cGenHashFunc<sHostObjID> cHostObjIDFunc;
   typedef cHashTable<sHostObjID *, ObjID, cHostObjIDFunc> cHashByHostObjID;

   cHashByHostObjID *m_pTable;
};

#endif // !_OBJPROXY_H
