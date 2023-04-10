/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkganpr.h,v 1.4 2000/01/31 09:55:47 adurant Exp $
// Shock gun animation properties
#pragma once

#ifndef __SHKGANPR_H
#define __SHKGANPR_H

#include <propface.h>
#include <gunanim.h>

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

#endif
