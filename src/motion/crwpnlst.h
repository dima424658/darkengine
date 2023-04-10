/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef _CRWPNLST_H
#define _CRWPNLST_H

#include <hashset.h>
#include <crwpnapi.h>

typedef cHashSet<sCreatureWeapon *, ObjID, cHashFunctions> cCreatureWeaponTableBase;

class cCreatureWeaponHash : public cCreatureWeaponTableBase
{
   virtual tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey)(((sCreatureWeapon *)(node))->obj);
   }
};

// Creature Weapon List init
void CreatureWeaponListInit(void);
void CreatureWeaponListTerm(void);

// Creature Weapon List accessors
sCreatureWeapon *CreatureWeaponGet(ObjID obj);

// Creature Weapon List manipulators
void CreatureWeaponAdd(ObjID obj);
void CreatureWeaponRem(ObjID obj);

#endif // _CRWPNLST_H
