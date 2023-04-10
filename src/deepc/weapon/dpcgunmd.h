/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCGUNMD_H
#define __DPCGUNMD_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

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
