/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/collide.h,v 1.17 1998/07/16 13:29:32 mahk Exp $

#ifndef __COLLIDE_H
#define __COLLIDE_H
#pragma once

#include <objtype.h>
#include <collprop.h>
#include <phclsn.h>
// collision testing return values

#define COLLISION_NONE           0

#define CF_US_SHF 0
#define CF_THEM_SHF 16

#define COLLISION_BOUNCE_US      (COLLISION_BOUNCE   << CF_US_SHF)
#define COLLISION_BOUNCE_THEM    (COLLISION_BOUNCE   << CF_THEM_SHF)
#define COLLISION_DAMAGE_US      (COLLISION_DAMAGE   << CF_US_SHF)
#define COLLISION_DAMAGE_THEM    (COLLISION_DAMAGE   << CF_THEM_SHF)
#define COLLISION_SLAY_US        (COLLISION_SLAY     << CF_US_SHF)
#define COLLISION_SLAY_THEM      (COLLISION_SLAY     << CF_THEM_SHF)
#define COLLISION_NO_SOUND_US    (COLLISION_NO_SOUND << CF_US_SHF)
#define COLLISION_NO_SOUND_THEM  (COLLISION_NO_SOUND << CF_THEM_SHF)
#define COLLISION_KILL_US        (COLLISION_KILL     << CF_US_SHF)
#define COLLISION_KILL_THEM      (COLLISION_KILL     << CF_THEM_SHF)
#define COLLISION_NOTHING_US     (COLLISION_NORESULT << CF_US_SHF)
#define COLLISION_NOTHING_THEM   (COLLISION_NORESULT << CF_THEM_SHF)

#define COLLISION_NON_PHYS       (1 << 12)
#define COLLISION_NON_PHYS_US    (COLLISION_NON_PHYS << CF_US_SHF)
#define COLLISION_NON_PHYS_THEM  (COLLISION_NON_PHYS << CF_THEM_SHF)


// don't send a network packet
#define COLLISION_LOCAL_ONLY           (1<<31)

class cPhysClsn; 

// return whether flags (KILL) for which were destroyed
extern int CollideEvent(ObjID obj1, ObjID obj2, int bits, float mag, cPhysClsn* clsn);

// test for the collision using the collision property, returns bits
extern int CollideTest(ObjID obj1, ObjID obj2);


#endif // __COLLIDE_H



