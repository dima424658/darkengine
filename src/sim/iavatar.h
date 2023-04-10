/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: r:/t2repos/thief2/src/sim/iavatar.h,v 1.4 1999/08/05 18:17:56 Justin Exp $
//
// Avatar interface
//
// in the new model, "avatars" as such only exist to track player num->objid mapping
//  - dc july 1

#ifndef __IAVATAR_H
#pragma once
#define __IAVATAR_H

#include <comtools.h>
#include <iobjsys.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(ITagFile);
F_DECLARE_INTERFACE(IAvatar);

#undef INTERFACE
#define INTERFACE IAvatar

DECLARE_INTERFACE_(IAvatar, IUnknown)
{
   // IUnknown methods
   DECLARE_UNKNOWN_PURE();

   // Get the player number corresponding to this avatar
   STDMETHOD_(ulong, PlayerNum) (THIS) PURE;

   // Get the player's object number
   STDMETHOD_(ObjID, PlayerObjID) (THIS) PURE;

   // Write the avatar's state information out to the given tag file
   // Assumes that a block to hold this avatar is already open, and
   // this just needs to write this avatar out.
   STDMETHOD_(void, WriteTagInfo) (THIS_ ITagFile *file) PURE;
};

#pragma pack()

#endif // !__IAVATAR_H
