/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkloop.h,v 1.3 2000/01/31 09:39:58 adurant Exp $
#pragma once
#ifndef __DRKLOOP_H
#define __DRKLOOP_H

////////////////////////////////////////////////////////////
// DARK-SPECIFIC LOOP CLIENTS
//

//
// DARK SIM CLIENT
// 
DEFINE_LG_GUID(LOOPID_DarkSim, 0x9d);
EXTERN struct sLoopClientDesc DarkSimLoopClientDesc;

//
// Dark RENDER CLIENT
// 
DEFINE_LG_GUID(LOOPID_DarkRender, 0x9e);
EXTERN struct sLoopClientDesc DarkRenderClientDesc;

////////////////////////////////////////////////////////////
// Dark LOOPMODES 
//
// These loopmodes are really just client sets.  They 
// are intended to be included by other loopmodes
// 

//
// Dark GAME MODE CLIENTS
// 
DEFINE_LG_GUID(LOOPID_DarkGameClients, 0x9f);
EXTERN struct sLoopModeDesc DarkGameClients; 

DEFINE_LG_GUID(LOOPID_DarkBaseClients, 0xA0);
EXTERN struct sLoopModeDesc DarkBaseClients;
//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __DRKLOOP_H
