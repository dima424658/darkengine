/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapcb.h,v 1.7 2000/01/31 09:45:50 adurant Exp $
//
// Weapon (hand-to-hand-combat) callback header
//
#pragma once

#ifndef __WEAPCB_H
#define __WEAPCB_H

#include <objtype.h>

////////////////////////////////////////

enum eWeaponEvent_
{
   kStartWindup  = 0x0001,
   kStartAttack  = 0x0002,
   kEndAttack    = 0x0004,
   kStartBlock   = 0x0008,
   kEndBlock     = 0x0010,
   kBlockEvent   = 0x0020,
   kHitEvent     = 0x0040,
   kStartRangedWindup  = 0x0100,
   kStartRangedAttack  = 0x0200,
   kEndRangedAttack    = 0x0400,
};

enum eWeaponEventType_
{
   kWeaponEventSmall   = 1,
   kWeaponEventMedium  = 2,
   kWeaponEventLarge   = 3,
   kWeaponEventBlock   = 4,
};

typedef int eWeaponEvent;
typedef int eWeaponEventType;

const int kNumWeaponEvents = 7;

////////////////////////////////////////

const eWeaponEvent kStartEndEvents =
  kStartWindup | kStartAttack | kEndAttack | kStartBlock | kEndBlock;

const eWeaponEvent kAllWeaponEvents =
  kStartWindup | kStartAttack | kEndAttack | kStartBlock | kEndBlock | kBlockEvent | kHitEvent;

////////////////////////////////////////

typedef void (*fWeaponEventCallback)(eWeaponEvent event, ObjID victim, ObjID culprit, void *data);

EXTERN void InitWeaponEventCallbacks();
EXTERN void TermWeaponEventCallbacks();

EXTERN BOOL RegisterWeaponEventCallback(eWeaponEvent events, fWeaponEventCallback callback, void *data);
EXTERN BOOL DeregisterWeaponEventCallback(eWeaponEvent events, fWeaponEventCallback callback);

EXTERN void WeaponEvent(eWeaponEvent event, ObjID culprit, ObjID victim, eWeaponEventType event_type = kWeaponEventMedium);

////////////////////////////////////////

#endif









