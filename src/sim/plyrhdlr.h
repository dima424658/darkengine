/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyrhdlr.h,v 1.9 2000/01/31 10:00:22 adurant Exp $
#pragma once

#ifndef __PLYRHDLR_H
#define __PLYRHDLR_H

#include <plyrtype.h>
#include <objtype.h>

class IPlayerLimbHandler
{
public:
   virtual BOOL Equip(ObjID item) = NULL;
   virtual void UnEquip(ObjID item) = NULL;
   virtual void StartUse(void) = NULL;
   virtual void FinishUse(void) = NULL;
};

EXTERN void PlayerHandlersInit(int nPlayerModes);
EXTERN void PlayerHandlerRegister(int mode, IPlayerLimbHandler *pHandler);

EXTERN void PlayerHandlersTerm();

EXTERN void PlayerHandlerSetFrameCallback(int mode, fPlayerFrameCallback callback, void *data);

EXTERN void PlayerHandlerSetEnterModeCallback(int mode, fPlayerModeCallback callback, void *data);
EXTERN void PlayerHandlerSetLeaveModeCallback(int mode, fPlayerModeCallback callback, void *data);
EXTERN void PlayerHandlerSetAbilityCallback(int mode, fPlayerAbilityCallback callback, void *data);

EXTERN void PlayerHandlerSetIdleModeCallback(int mode, fPlayerModeCallback callback, void *data);

// These just report events
EXTERN void PlayerHandlerIdleMode(int mode);

EXTERN BOOL PlayerHandlerEquip(int mode, ObjID item);
EXTERN BOOL PlayerHandlerUnEquip(int mode, ObjID item);
EXTERN BOOL PlayerHandlerStartUse(int mode);
EXTERN BOOL PlayerHandlerFinishUse(int mode);


#endif


