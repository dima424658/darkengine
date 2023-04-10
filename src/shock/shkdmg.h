/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdmg.h,v 1.3 2000/01/31 09:55:37 adurant Exp $
// Shock damage models
#pragma once

#ifndef __SHKDMG_H
#define __SHKDMG_H

#include <dmgmodel.h>

EXTERN void ShockDamageInit(void);
EXTERN void ShockDamageShutDown(void);

EXTERN eDamageResult LGAPI ShockDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data);

#endif