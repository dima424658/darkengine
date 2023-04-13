/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkwepsc.h,v 1.4 2000/01/31 09:59:35 adurant Exp $
//
// psi script stuff
//
#pragma once

#ifndef __SHKWEPSC_H
#define __SHKWEPSC_H

#include <scrptmsg.h>
#include <engscrpt.h>

#include <fix.h>

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

DECLARE_SCRIPT_SERVICE(ShockWeapon, 0x213)
{
   STDMETHOD_(void, SetWeaponModel)(const object& obj) PURE;
   STDMETHOD_(object, GetWeaponModel)(void) PURE;

   STDMETHOD_(object, TargetScan)(object projectile) PURE;
   STDMETHOD_(void, Home)(object projectile, object target) PURE;

   STDMETHOD_(void, DestroyMelee)(const object& obj) PURE;
};

#endif /* !__SHKWEPSC_H */
