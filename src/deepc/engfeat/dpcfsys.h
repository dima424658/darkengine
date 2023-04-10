/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCFSYS_H
#define __DPCFSYS_H

#ifndef _DPCNET_H
#include <dpcnet.h>
#endif // !_DPCNET_H

void DPCFrobsysInit(void);
void DPCFrobsysTerm(void);
void DPCDoFrob(bool in_inv);

// Returns TRUE iff we're waiting for the host to reply to some frob
// requests:
BOOL DPCFrobPending();

#endif  // __DPCFSYS_H
