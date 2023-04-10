/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objproxy.cpp,v 1.2 1998/11/18 15:32:50 JUSTIN Exp $
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
// host objIDs to the local proxies.
//

#include <lg.h>

#include <objproxy.h>

// Must be last:
#include <dbmem.h>

cProxyManager::cProxyManager()
{
   Warning(("cProxyManager shouldn't exist any longer!\n"));
}

cProxyManager::~cProxyManager()
{
}

