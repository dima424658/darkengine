/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/bowscrpt.cpp,v 1.4 1998/10/22 00:10:48 dc Exp $

#include <lg.h>
#include <playrobj.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <bowscrpt.h>
#include <weapon.h>
#include <drkwbow.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Weapon Script Service

DECLARE_SCRIPT_SERVICE_IMPL(cBowSrv, Bow)
{
public:

   STDMETHOD(Equip)()
   {
      return EquipBow();
   }

   STDMETHOD(UnEquip)()
   {
      return UnEquipBow();
   }

   STDMETHOD_(BOOL, IsEquipped)()
   {
      return IsBowEquipped();
   }

   STDMETHOD(StartAttack)()
   {
      StartBowAttack();
      return TRUE;
   }

   STDMETHOD(AbortAttack)()
   {
      return AbortBowAttack();
   }

   STDMETHOD(FinishAttack)()
   {
      return FinishBowAttack();
   }

   STDMETHOD_(BOOL, SetArrow)(object arrow)
   {
      return SetCurrentArrow(arrow);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cBowSrv, Bow);















