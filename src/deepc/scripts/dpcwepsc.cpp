/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcwepsc.h>

#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <playrobj.h>
#include <weapon.h>

#include <dpchome.h>

#include <dpcmelee.h>

#include <dpcpgapi.h>
#include <dpcplayr.h>

// must be last header
#include <dbmem.h>

DECLARE_SCRIPT_SERVICE_IMPL(cDPCWeaponSrv, DPCWeapon)
{
public:
   STDMETHOD_(void, SetWeaponModel)(const object& obj)
   {
      AutoAppIPtr(DPCPlayer);
      pDPCPlayer->SetWeaponModel(obj);
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
   STDMETHOD_(void, DestroyMelee)(const object& obj)
   {
      ::DestroyMelee(ObjID(obj), PlayerObject());
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDPCWeaponSrv, DPCWeapon);
