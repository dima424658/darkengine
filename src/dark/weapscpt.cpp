/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/weapscpt.cpp,v 1.4 1998/07/25 12:52:35 CCAROLLO Exp $

#include <lg.h>
#include <playrobj.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <weapscpt.h>
#include <drkwswd.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Weapon Script Service

DECLARE_SCRIPT_SERVICE_IMPL(cWeaponSrv, Weapon)
{
public:

   STDMETHOD(Equip)(object weapon, int type)
   {
      return EquipWeapon(PlayerObject(), weapon, type);
   }

   STDMETHOD(UnEquip)(object weapon)
   {
      return UnEquipWeapon(PlayerObject(), weapon);
   }

   STDMETHOD_(BOOL, IsEquipped)(object owner, object weapon)
   {
      return IsWeaponEquipped(owner, weapon);
   }

   STDMETHOD(StartAttack)(object owner, object weapon)
   {
      StartWeaponAttack(owner, weapon);
      return TRUE;
   }

   STDMETHOD(FinishAttack)(object owner, object weapon)
   {
      FinishWeaponAction(owner, weapon);
      return TRUE;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cWeaponSrv, Weapon);











