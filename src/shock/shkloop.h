/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkloop.h,v 1.5 2000/01/31 09:57:51 adurant Exp $
#pragma once
#ifndef __SHKLOOP_H
#define __SHKLOOP_H

////////////////////////////////////////////////////////////
// SHOCK-SPECIFIC LOOP CLIENTS
//

//
// SHOCK SIM CLIENT
// 
DEFINE_LG_GUID(LOOPID_ShockSim, 0xc2);
EXTERN struct sLoopClientDesc ShockSimLoopClientDesc;

//
// SHOCK RENDER CLIENT
// 
DEFINE_LG_GUID(LOOPID_ShockRender, 0xc3);
EXTERN struct sLoopClientDesc ShockRenderClientDesc;

//DEFINE_LG_GUID(LOOPID_ShockRender2, 0x130);
//EXTERN struct sLoopClientDesc ShockRender2ClientDesc;

////////////////////////////////////////////////////////////
// SHOCK LOOPMODES 
//
// These loopmodes are really just client sets.  They 
// are intended to be included by other loopmodes
// 

//
// SHOCK GAME MODE CLIENTS
// 
DEFINE_LG_GUID(LOOPID_ShockGameClients, 0xc4);
EXTERN struct sLoopModeDesc ShockGameClients; 

DEFINE_LG_GUID(LOOPID_ShockBaseClients, 0xc5);
EXTERN struct sLoopModeDesc ShockBaseClients;
//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __AIRLOOP_H
