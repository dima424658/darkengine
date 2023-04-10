/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netapp.h,v 1.2 2000/01/29 13:21:23 adurant Exp $
//
// Network Application Services
//
// This interface is intended to pull together all functions that
// networking expects from the specific game. As of this writing, we
// haven't made a principled pass to make it complete, but it's the
// right place to put any new dependencies from netman to the game.
//
// Note that it is the responsibility of any networkable game to provide
// an implementation of these services.
//
#pragma once

#ifndef __NETAPP_H
#define __NETAPP_H

#include <comtools.h>
#include <objtype.h>

F_DECLARE_INTERFACE(INetAppServices);

#undef INTERFACE
#define INTERFACE INetAppServices

DECLARE_INTERFACE_( INetAppServices, IUnknown )
{
   DECLARE_UNKNOWN_PURE();

   // This method should return the objID of an object to use as the
   // current player's avatar.
   STDMETHOD_(ObjID, MyAvatarArchetype)(THIS) PURE;
};

#endif // !__NETAPP_H
