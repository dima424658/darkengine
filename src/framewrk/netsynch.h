/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netsynch.h,v 1.6 1999/08/05 17:03:17 Justin Exp $

#pragma once  
#ifndef __NETSYNCH_H
#define __NETSYNCH_H

#include <looptype.h>

////////////////////////////////////////////////////////////
// NETWORK SYNCHRONIZATION
//

#ifdef NEW_NETWORK_ENABLED
EXTERN void NetSynchInit(void); 
EXTERN void NetSynchTerm(void); 

//
// Turn network synchronization off and on. By default, it is on; by
// calling NetSynchDisable(), you declare that synchronization won't
// happen (and that this level will not be networked).
//
// These *must* be called between resetting the database and loading
// the new level, so that all of the messages for the level are properly
// sent or suppressed, as appropriate.
//
EXTERN void NetSynchDisable(void);
EXTERN void NetSynchEnable(void);
EXTERN BOOL IsNetSynchEnabled(); 

//
// A silly little hack, put in place to disable the remaps that may
// occur after we re-enable netsynch.
//
EXTERN void NetSynchDisableRemapMsgs(void);
EXTERN void NetSynchEnableRemapMsgs(void);
EXTERN BOOL NetSynchRemapMsgsDisabled(void);
#endif

//
// Synchronize with other players
//
// If NEW_NETWORK_ENABLED is not on, this simply switches to game mode
// (for simplicity of Shock code).
//
EXTERN void SwitchToNetSynchMode(eLoopModeChangeKind how); 

//
// Set the mode to use as our net synch mode, NULL is the default synch mode
//
EXTERN void SetNetSynchMode(const struct sLoopInstantiator* mode); 

#endif // !__NETSYNCH_H



