/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/collprop.h,v 1.9 2000/01/29 13:32:33 adurant Exp $
#pragma once

//
// Collision property header
//

#ifndef __COLLPROP_H
#define __COLLPROP_H

#include <objtype.h>

#define PROP_COLLISION_NAME "CollisionType"

// collision property bitfield values
#define COLLISION_NONE           0
#define COLLISION_BOUNCE         (1<<0)
#define COLLISION_KILL           (1<<1)
#define COLLISION_SLAY           (1<<2)
#define COLLISION_NO_SOUND       (1<<3)
#define COLLISION_NORESULT       (1<<4)
#define COLLISION_FULL_SOUND     (1<<5)

EXTERN void CollisionTypePropInit(void);
EXTERN BOOL ObjGetCollisionType(ObjID obj, int *num);
EXTERN void ObjSetCollisionType(ObjID obj, int num);

#endif   // __COLLPROP_H
