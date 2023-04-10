/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkarmor.h,v 1.3 2000/01/29 13:40:28 adurant Exp $
#pragma once

#ifndef __SHKARMOR__H
#define __SHKARMOR__H

#include <dmgmodel.h>

eDamageResult LGAPI ShockArmorFilter(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData data);

void ShockArmorInit(void);
void ShockArmorTerm(void);
void ShockArmorStartGamemode(void);

#endif // __SHKARMOR__H
