/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/basemode.h,v 1.4 2000/01/29 13:20:20 adurant Exp $
#pragma once
#ifndef __BASEMODE_H
#define __BASEMODE_H



DEFINE_LG_GUID(LOOPID_BaseMode, 0x2e);

EXTERN struct sLoopModeDesc BaseLoopMode;


//
// Base loop client
//

DEFINE_LG_GUID(LOOPID_Base, 0x2f);
EXTERN struct sLoopClientDesc  BaseLoopClientDesc;


//
// Plug the gamespec client (or mode) into the base mode.  Only the last call is relevant.
//

EXTERN void BaseModeSetGameSpecClient(const GUID* clientID);
 
#endif // __BASEMODE_H
