/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkloot.h,v 1.3 2000/01/31 09:57:52 adurant Exp $
#pragma once

#ifndef __SHKLOOT_H
#define __SHKLOOT_H

#include <dmgmodel.h>

EXTERN eDamageResult LGAPI ShockLootDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data);

EXTERN void GenerateLoot(ObjID corpse);

#endif