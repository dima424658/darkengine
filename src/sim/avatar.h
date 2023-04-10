/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: r:/t2repos/thief2/src/sim/avatar.h,v 1.6 1999/08/05 18:04:44 Justin Exp $
//
// in the new model, "avatars" as such only exist to track player num->objid mapping
//  - dc july 1

#ifndef _AVATAR_H
#pragma once
#define _AVATAR_H

#include <iavatar.h>
#include <tagfile.h>

class cAvatar : public IAvatar
{
public:
   DECLARE_UNAGGREGATABLE();

   cAvatar(ObjID player, ulong playerNum);

   // Alternate constructor, to re-create an avatar that was written out
   // with WriteTagInfo. Assumes that the block is open, and pointing to
   // the avatar info.
   cAvatar(ITagFile *file);

   // Get the player number corresponding to this avatar
   STDMETHOD_(ulong, PlayerNum) ();

   // Get the player's object number
   STDMETHOD_(ObjID, PlayerObjID) ();

   // Write the avatar's state information out to the given tag file
   STDMETHOD_(void, WriteTagInfo) (ITagFile *file);

protected:
   ulong m_playerNum;     // The 1 to N ordinal player number
   ObjID m_player;      // the object for this avatar's player
};

#endif // !_AVATAR_H
