/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover melee weapon related properties
#pragma once

#ifndef __DPCMELPR_H
#define __DPCMELPR_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

// This is a little confusing
// Melee type specifies that this object is a melee weapon and
// determines which player arm "skill set" is used
// It is different from "weapon type".

typedef enum eMeleeType {
   kMeleeKnife,
   kMeleeKnuckles,

   kMeleePad = 0xFFFFFFFF,
};

#define PROP_DPC_MELEE_TYPE_NAME "Melee Type"
EXTERN IIntProperty* g_pMeleeTypeProperty;
EXTERN void MeleeTypePropertyInit(void);
EXTERN void MeleeTypePropertyShutdown(void);
#endif
