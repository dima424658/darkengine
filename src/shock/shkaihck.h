/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkaihck.h,v 1.3 2000/01/29 13:40:17 adurant Exp $
#pragma once

#ifndef __SHKAIHCK
#define __SHKAIHCK

#include <propface.h>
#include <ai.h>

enum EShockAIType {kShockAINone = 0, kShockAITurret = 1, kShockAIDroid = 2, kShockAIGrunt = 3, };

// Hacked AI Type
#define PROP_SHOCK_AI_TYPE_NAME "Shock AI"
EXTERN  IIntProperty* gPropShockAIType;
EXTERN  void ShockAITypePropInit();
EXTERN  int GetShockAIType(ObjID obj);

void ShockAIDie(cAI *pAI, int type);

#endif