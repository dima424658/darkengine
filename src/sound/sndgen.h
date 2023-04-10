/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndgen.h,v 1.10 2000/03/05 18:37:43 adurant Exp $

#pragma once
#ifndef __SNDGEN_H
#define __SNDGEN_H

#include <objtype.h>
#include <propface.h>
#include <matrixs.h>

EXTERN void CollisionSoundObjects(ObjID obj1ID, ObjID obj2ID, int collisionResult, float mag);
EXTERN void FootfallSound(const ObjID objID, const mxs_vector *footPos);
EXTERN void WeaponChargeSound(const ObjID objID, const mxs_vector *weaponPos);
EXTERN void WeaponSwingSound(const ObjID objID, const mxs_vector *weaponPos);
EXTERN void SearchSound(const ObjID objID, const mxs_vector *searchPos);
EXTERN void SoundGenInit(void);
EXTERN void SoundGenShutdown(void);



#endif


