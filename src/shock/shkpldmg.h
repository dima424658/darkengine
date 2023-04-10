/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpldmg.h,v 1.3 2000/01/31 09:58:32 adurant Exp $
#pragma once

#ifndef __SHKPLDMG_H
#define __SHKPLDMG_H

#include <dmgmodel.h>

eDamageResult LGAPI ShockPlayerDamageFilter(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData data);

void ShockEquipArmor(ObjID equipperID, ObjID armorID);
void ShockUnequipArmor(ObjID equipperID, ObjID armorID);

void ShockPlayerDamageInit(void);
void ShockPlayerDamageTerm(void);

#endif // __SHKPLDMG_H
