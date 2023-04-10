/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyrtype.h,v 1.12 2000/01/31 10:00:27 adurant Exp $
#pragma once

#ifndef __PLYRTYPE_H
#define __PLYRTYPE_H

typedef int tPlayerActionType;

#define kPlayerModeInvalid 0xff

#define kPlayerActionTypeInvalid 0xff

typedef BOOL (*fPlayerFrameCallback)(ulong dt, void *data);

typedef void (*fPlayerIdleCallback)(int mode);

typedef void (*fPlayerModeCallback)(int mode, void *data);

typedef void (*fPlayerModeChangeCallback)(int mode);

#ifdef __cplusplus
typedef class cPlayerAbility cPlayerAbility;
typedef class IPlayerLimbHandler IPlayerLimbHandler;

// callback should set ability pointer appropriately
typedef void (*fPlayerAbilityCallback)(int mode, cPlayerAbility **pAbility, void *data);

#endif // cplusplus

#endif
