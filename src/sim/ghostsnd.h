/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostsnd.h,v 1.2 2000/01/29 13:41:12 adurant Exp $
// send packets/local control systems for ghosts
#pragma once

#ifndef __GHOSTSND_H
#define __GHOSTSND_H

#include <ghosttyp.h>

// actually run a frame of the local ghost control/examination stuff
EXTERN void _GhostFrameProcessLocal(sGhostLocal *pGL, float dt);

// init/term the ghost send stuff, really just to get the appagg for the AI
EXTERN void GhostSendInit(void);
EXTERN void GhostSendTerm(void);

#endif  // __GHOSTSND_H
