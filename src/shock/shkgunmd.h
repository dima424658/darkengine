/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgunmd.h,v 1.4 2000/01/31 09:56:32 adurant Exp $
#pragma once

#ifndef __SHKGUNMD_H
#define __SHKGUNMD_H

#include <objtype.h>

EXTERN void PlayerGunModelOn(ObjID gunID);
EXTERN void PlayerGunModelPosUpdate(void);
EXTERN void PlayerGunModelOff(void);   // turn gun off by lowering it
EXTERN void PlayerGunModelKill(void);  // turn gun off instantly
EXTERN void PlayerGunModelUpdate(void);
EXTERN void PlayerGunModelInit(void);
EXTERN void PlayerGunModelShutdown(void);
EXTERN BOOL PlayerGunModelIsOn(void);
// the model controls firing
EXTERN BOOL PlayerGunModelFire(void);

#endif
