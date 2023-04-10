/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/weapscpt.h,v 1.2 1998/06/12 11:57:47 kate Exp $
#pragma once  
#ifndef __WEAPSCPT_H
#define __WEAPSCPT_H

#include <objscrpt.h>

///////////////////////////////////////
//
// Weapon system service 
//

enum eDarkWeaponType
{
   kDWT_Sword,
   kDWT_BlackJack,
};

DECLARE_SCRIPT_SERVICE(Weapon, 0x10e)
{
   // Equip the specified weapon
   STDMETHOD(Equip)(object weapon, int type=kDWT_Sword) PURE;
   // Put away the specified weapon
   STDMETHOD(UnEquip)(object weapon) PURE;

   // Has the specified weapon finished being equipped?
   STDMETHOD_(BOOL, IsEquipped)(object owner, object weapon) PURE;

   // Start an attack (frob start)
   STDMETHOD(StartAttack)(object owner, object weapon) PURE;
   // Finish an attack (frob end)
   STDMETHOD(FinishAttack)(object owner, object weapon) PURE;
};

#endif  // __WEAPSCPT_H





