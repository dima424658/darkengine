/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctagnet.h,v 1.4 2000/01/29 13:20:28 adurant Exp $
//
// Networking for the cTags array.
//
// @NOTE (justin 6/15/99): Since no one seems to be using tags networking
// currently, and it seems to be linked to a rare hang at synch time,
// I've disabled it for the time being.
//
#pragma once

#ifndef __CTAGNET_H
#define __CTAGNET_H

#ifdef TAG_NETWORKING_ON

#include <objtype.h>
#include <ctag.h>
#include <ctagset.h>

//////////
//
// Functions to translate a cTagSet into and out of a compact block,
// suitable for networking.
//
// These are the only routines that ordinary Dark systems should be
// calling routinely. They are intended for use within routines that
// are sending and receiving network messages that need to include
// cTagSets.
//

//
// Given a cTagSet, return a block suitable for a network message. It
// is the caller's responsibility to free the block when finished.
//
// Returns the size of the block in bytes.
//
EXTERN int TagSetToNetBlock(const cTagSet *pTagSet, void **ppBlock);

//
// Given a block received over the network, and the player it came from,
// and an empty cTagSet, fill the TagSet with the information in the
// block.
//
EXTERN void NetBlockToTagSet(void *pBlock, ObjID player, cTagSet *pTagSet);

//////////
//
// Functions for keeping the players in synch. These are only used within
// the tag system.
//

//
// Send out a new tag record to the other players
//
EXTERN void BroadcastTagRecord(cTagRecord *pRecord);

//
// Send out a new enumeration value to the other players
//
EXTERN void BroadcastTagEnum(cTagRecord *pRecord, 
                             int index, 
                             const char *pszValueName);

//////////
//
// Initialize and shut down the networking.
//
EXTERN void TagsNetInit();
EXTERN void TagsNetTerm();

#else // !TAG_NETWORKING_ON

// Stubs for all of the functions; we'll leave the code in place, in
// case we decide to reactivate it later:
#define TagSetToNetBlock(a, b) CriticalMsg("Tag networking disabled!");
#define NetBlockToTagSet(a, b, c) CriticalMsg("Tag networking disabled!");
#define BroadcastTagRecord(a)
#define BroadcastTagEnum(a, b, c)
#define TagsNetInit()
#define TagsNetTerm()

#endif // TAG_NETWORKING_ON

#endif // !__CTAGNET_H
