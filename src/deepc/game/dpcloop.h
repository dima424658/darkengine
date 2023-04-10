/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCLOOP_H
#define __DPCLOOP_H

////////////////////////////////////////////////////////////
// DEEP COVER-SPECIFIC LOOP CLIENTS
//

//
// DEEP COVER SIM CLIENT
// 
DEFINE_LG_GUID(LOOPID_DPCSim, 0x349);
EXTERN struct sLoopClientDesc DPCSimLoopClientDesc;

//
// DEEP COVER RENDER CLIENT
// 
DEFINE_LG_GUID(LOOPID_DPCRender, 0x34a);
EXTERN struct sLoopClientDesc DPCRenderClientDesc;

//DEFINE_LG_GUID(LOOPID_DPCRender2, 0x130);
//EXTERN struct sLoopClientDesc DPCRender2ClientDesc;

////////////////////////////////////////////////////////////
// DEEP COVER LOOPMODES 
//
// These loopmodes are really just client sets.  They 
// are intended to be included by other loopmodes
// 

//
// DEEP COVER GAME MODE CLIENTS
// 
DEFINE_LG_GUID(LOOPID_DPCGameClients, 0x34b);
EXTERN struct sLoopModeDesc DPCGameClients; 

DEFINE_LG_GUID(LOOPID_DPCBaseClients, 0x34c);
EXTERN struct sLoopModeDesc DPCBaseClients;
//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

#endif // __DPCLOOP_H
