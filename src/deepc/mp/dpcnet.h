/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef _DPCNET_H
#define _DPCNET_H

#ifndef ___LG_H
#include <lg.h>
#endif // !___LG_H

#ifndef __DPCOVCST_H
#include <dpcovcst.h>
#endif // __DPCOVCST_H

//////////
//
// Utility methods
//
// Display the given text on the given player's HUD. If player is OBJ_NULL,
// then do so on all players except this one. If player is the current
// player, message will get short-circuited through.
// 
EXTERN void DPCSendAddText(ObjID player, 
                             const char *pText, 
                             int time = DEFAULT_MSG_TIME);

//
// Tell all the other machines to remove the specified object from the
// specified container.
//
EXTERN void DPCBroadcastRemoveContainee(ObjID o, ObjID cont);

//////////
//
// Host name
//
// A name to distinctly identify this host. Should be player-assigned.
// DPCGetHostName() is guaranteed to return a non-NULL pointer. Legal
// characters are restricted to alphanumeric, space, and underscore,
// since this will be used to construct a directory.
//
#define MAX_HOSTNAME_LEN 16
EXTERN void DPCSetHostName(const char *name);
EXTERN const char *DPCGetHostName();

//////////
//
// Player color
//
// Return the color appropriate for identifying this player.
//
// By player number:
EXTERN int DPCPlayerNumColor(int playerNum);
// By player objID:
EXTERN int DPCPlayerColor(ObjID player);

//////////
//
// Hilight an object
//
// Shows the HUD bars around a given object on all machines, including
// this one.
//
EXTERN void DPCBroadcastHilightObject(ObjID obj);

//////////
//
// Startup and Shutdown
//
EXTERN void DPCNetInit();
EXTERN void DPCNetTerm();

#endif // !_DPCNET_H
