/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover gun animation properties
#pragma once

#ifndef __DPCGANPR_H
#define __DPCGANPR_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

#ifndef __GUNANIM_H
#include <gunanim.h>
#endif // !__GUNANIM_H

// Gun Anim 
#undef INTERFACE
#define INTERFACE IGunAnimProperty
DECLARE_PROPERTY_INTERFACE(IGunAnimProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sGunAnim*); 
}; 

#define PROP_GUN_ANIM_POST_NAME "AnimPost"
#define PROP_GUN_ANIM_PRE_NAME "AnimPre"

extern IGunAnimProperty *g_gunAnimPreProperty;
extern IGunAnimProperty *g_gunAnimPostProperty;

void GunAnimPropertyInit(void);
void GunAnimPropertyTerm(void);

#endif  // !__DPCGANPR_H