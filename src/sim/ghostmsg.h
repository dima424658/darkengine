/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostmsg.h,v 1.6 2000/01/29 13:41:11 adurant Exp $
// message types and declarations
#pragma once

#ifndef __GHOSTMSG_H
#define __GHOSTMSG_H

#include <netmsg.h>

// actual messages
EXTERN cNetMsg *g_pGhostHeartbeatFullMsg;
EXTERN cNetMsg *g_pGhostHeartbeatNormMsg;
EXTERN cNetMsg *g_pGhostHeartbeatTerseNGMsg;
EXTERN cNetMsg *g_pGhostMiniHeartbeatNGMsg;
EXTERN cNetMsg *g_pGhostObjHeartbeatNGMsg;
EXTERN cNetMsg *g_pGhostAIHeartbeatNGMsg;
EXTERN cNetMsg *g_pGhostRotHeartbeatNGMsg;

// actual message create/destroy
EXTERN void GhostMsgsInit(void);
EXTERN void GhostMsgsTerm(void);

#endif
