/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkctrl.h,v 1.7 2000/01/31 09:55:29 adurant Exp $
#pragma once

#ifndef __SHKCTRL_H
#define __SHKCTRL_H

// This is mostly redundant - it's currently only used by the under-utilized camera control system
EXTERN bool ShockControlKeyParse(int keycode, ObjID controlObj, float speed);
EXTERN void ShockPlayerJump(void);

#endif
