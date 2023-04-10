/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Weapon script stuff
//
#pragma once

#ifndef __DPCWEPSC_H
#define __DPCWEPSC_H

#include <scrptmsg.h>
#include <engscrpt.h>

#include <fix.h>

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

DECLARE_SCRIPT_SERVICE(DPCWeapon, 0x213)
{
   STDMETHOD_(void, SetWeaponModel)(const object ref obj) PURE;
   STDMETHOD_(object, GetWeaponModel)(void) PURE;

   STDMETHOD_(object, TargetScan)(object projectile) PURE;
   STDMETHOD_(void, Home)(object projectile, object target) PURE;

   STDMETHOD_(void, DestroyMelee)(const object ref obj) PURE;
};

#endif /* !__DPCWEPSC_H */
