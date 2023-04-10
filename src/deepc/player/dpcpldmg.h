/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPLDMG_H
#define __DPCPLDMG_H

#ifndef __DMGMODEL_H
#include <dmgmodel.h>
#endif // ! __DMGMODEL_H
eDamageResult LGAPI DPCPlayerDamageFilter(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData data);

void DPCEquipArmor(ObjID   equipperID, ObjID armorID);
void DPCUnequipArmor(ObjID equipperID, ObjID armorID);

void DPCPlayerDamageInit(void);
void DPCPlayerDamageTerm(void);

#endif // __DPCPLDMG_H
