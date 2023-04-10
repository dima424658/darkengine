/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// projectile related properties
#pragma once

#ifndef __PROJPROP_H
#define __PROJPROP_H

#include <propface.h>
#include <projbase.h>

F_DECLARE_INTERFACE(IProjectileProperty);

#undef INTERFACE
#define INTERFACE IProjectileProperty

DECLARE_PROPERTY_INTERFACE(IProjectileProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sProjectile*);  // Type-specific accessors, by reference
};

#define PROP_PROJECTILE_DESC "Projectile"

EXTERN IProjectileProperty *g_pProjectileProperty;

EXTERN void ProjectilePropertyInit();
EXTERN BOOL ProjectileGet(ObjID objID, sProjectile **ppProjectile);
#define PROJECTILE_DESC_SET(objID, ppParams) \
   PROPERTY_SET(g_projectileProperty, objID, (sProjectile**)ppParams)
#define PROJECTILE_DESC_DELETE(objID) \
   IProperty_Delete(g_projectileProperty, objID)

#endif //!__PROJPROP_H
