/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shknet.h,v 1.18 2000/01/31 09:58:08 adurant Exp $
#pragma once

#ifndef _SHKNET_H
#define _SHKNET_H

#include <lg.h>
#include <shkovcst.h>

//////////
//
// Utility methods
//
// Display the given text on the given player's HUD. If player is OBJ_NULL,
// then do so on all players except this one. If player is the current
// player, message will get short-circuited through.
// 
EXTERN void ShockSendAddText(ObjID player, 
                             const char *pText, 
                             int time = DEFAULT_MSG_TIME);

//
// Tell all the other machines to remove the specified object from the
// specified container.
//
EXTERN void ShockBroadcastRemoveContainee(ObjID o, ObjID cont);

//////////
//
// Host name
//
// A name to distinctly identify this host. Should be player-assigned.
// ShockGetHostName() is guaranteed to return a non-NULL pointer. Legal
// characters are restricted to alphanumeric, space, and underscore,
// since this will be used to construct a directory.
//
#define MAX_HOSTNAME_LEN 16
EXTERN void ShockSetHostName(const char *name);
EXTERN const char *ShockGetHostName();

//////////
//
// Player color
//
// Return the color appropriate for identifying this player.
//
// By player number:
EXTERN int ShockPlayerNumColor(int playerNum);
// By player objID:
EXTERN int ShockPlayerColor(ObjID player);

//////////
//
// Hilight an object
//
// Shows the HUD bars around a given object on all machines, including
// this one.
//
EXTERN void ShockBroadcastHilightObject(ObjID obj);

//////////
//
// Startup and Shutdown
//
EXTERN void ShockNetInit();
EXTERN void ShockNetTerm();

#endif // !_SHKNET_H
