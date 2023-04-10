/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCTRL_H
#define __DPCCTRL_H

#ifndef _OBJTYPE_H
#include "objtype.h"
#endif // !_OBJTYPE_H

// This is mostly redundant - it's currently only used by the under-utilized camera control system
EXTERN bool DPCControlKeyParse(int keycode, ObjID controlObj, float speed);
EXTERN void DPCPlayerJump(void);

#endif  // __DPCCTRL_H
