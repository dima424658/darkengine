/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostrcv.h,v 1.5 1999/08/05 18:14:27 Justin Exp $
// receive systems for ghosts

#ifndef __GHOSTRCV_H
#pragma once
#define __GHOSTRCV_H

#include <ghosttyp.h>

// per frame call from the core
EXTERN void _GhostFrameProcessRemote(sGhostRemote *pGR, float dt);

// called from callbacks on packet arrivals
EXTERN void  GhostRecvPacket(ObjID ghost, int seq_id, ObjID rel_obj, sGhostHeartbeat *pGH, sGhostMoCap *pGMC);

#endif  // __GHOSTRCV_H
