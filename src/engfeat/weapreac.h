/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapreac.h,v 1.5 2000/01/31 09:45:57 adurant Exp $
//
// Weapon (hand-to-hand-combat) reaction header
//
#pragma once

#ifndef __WEAPREAC_H
#define __WEAPREAC_H

#include <reaction.h>

#define REACTION_WEAPON_HIT   "weapon_hit"
#define REACTION_WEAPON_BLOCK "weapon_block"

EXTERN void InitWeaponReactions();
EXTERN void InitDamageListener();


#endif // __WEAPREAC_H
