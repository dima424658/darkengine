/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmelpr.h,v 1.2 2000/01/31 09:57:57 adurant Exp $
// Shock melee weapon related properties
#pragma once

#ifndef __SHKMELPR_H
#define __SHKMELPR_H

#include <propface.h>

// This is a little confusing
// Melee type specifies that this object is a melee weapon and
// determines which player arm "skill set" is used
// It is different from "weapon type".

#define PROP_SHOCK_MELEE_TYPE_NAME "Melee Type"
EXTERN IIntProperty* g_pMeleeTypeProperty;
EXTERN void MeleeTypePropertyInit();

#endif