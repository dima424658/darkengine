/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkwepsc.cpp,v 1.3 1999/04/07 13:43:49 JON Exp $

#include <shkwepsc.h>

#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <playrobj.h>
#include <weapon.h>

#include <shkhome.h>

#include <shkmelee.h>

#include <shkpgapi.h>
#include <shkplayr.h>

// must be last header
#include <dbmem.h>

DECLARE_SCRIPT_SERVICE_IMPL(cShockWeaponSrv, ShockWeapon)
{
public:
   STDMETHOD_(void, SetWeaponModel)(const object ref obj)
   {
      AutoAppIPtr(ShockPlayer);
      pShockPlayer->SetWeaponModel(obj);
   }
   STDMETHOD_(object, GetWeaponModel)(void)
   {
      ObjID objID;

      objID = GetWeaponObjID(PlayerObject());
      if (objID == OBJ_NULL)
      {
         AutoAppIPtr(PlayerGun);
         objID = pPlayerGun->Get();
      }
      return objID;
   }
   STDMETHOD_(object, TargetScan)(object projectile)
   {
      return ::TargetScan(projectile);
   }
   STDMETHOD_(void, Home)(object projectile, object target)
   {
      ::Home(projectile, target);
   }
   STDMETHOD_(void, DestroyMelee)(const object ref obj)
   {
      ::DestroyMelee(ObjID(obj), PlayerObject());
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cShockWeaponSrv, ShockWeapon);
