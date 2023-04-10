/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkfsys.h,v 1.6 2000/01/31 09:55:45 adurant Exp $
#pragma once

#ifndef __SHKFSYS_H
#define __SHKFSYS_H

#include <shknet.h>

void ShockFrobsysInit(void);
void ShockFrobsysTerm(void);
void ShockDoFrob(bool in_inv);

// Returns TRUE iff we're waiting for the host to reply to some frob
// requests:
BOOL ShockFrobPending();

#endif
