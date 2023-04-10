/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCLOOT_H
#define __DPCLOOT_H

#ifndef _DMGMODEL_H
#include <dmgmodel.h>
#endif // !_DMGMODEL_H

EXTERN eDamageResult LGAPI DPCLootDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data);

EXTERN void GenerateLoot(ObjID corpse);

#endif  // __DPCLOOT_H