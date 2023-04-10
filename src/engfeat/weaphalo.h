/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weaphalo.h,v 1.6 2000/03/17 12:38:19 bfarquha Exp $
//
// Weapon halo header
//
#pragma once

#ifdef THIEF

#include <objtype.h>
#include <matrixs.h>


#define kWH_NumModels  8
#define kWH_Set2Start  4
#define kWH_Set2Part2Start  6

#define kWH_PlayerRadius   1.0
#define kWH_PlayerOffset   1.3
#define kWH_PlayerPushout  2.0

#define kWH_PlayerRadius2   2.0
#define kWH_PlayerOffset2   1.0
#define kWH_PlayerPushout2  0.0
#define kWH_PlayerSet2Offset -1.3

#define kWH_AIRadius   1.4
#define kWH_AIOffset   2.0
#define kWH_AIPushout  3.0

////////////////////////////////////////

EXTERN void InitWeaponHaloSystem();
EXTERN void TermWeaponHaloSystem();

////////////////////////////////////////

EXTERN BOOL WeaponHaloInflated(ObjID weapon);
EXTERN BOOL WeaponSubmodIsHalo(ObjID weapon, int submod);

EXTERN BOOL WeaponHaloIsBlocking(ObjID objID);
EXTERN BOOL WeaponGetBlockingHalo(ObjID objID, int *haloId);

EXTERN mxs_real WeaponHaloQuadDist(ObjID objID, int quad_id, mxs_vector &subPos);

////////////////////////////////////////

EXTERN void WeaponHaloInit(ObjID objID, ObjID weapon, int numSubModels);
EXTERN void WeaponHaloTerm(ObjID objID, ObjID weapon);

EXTERN void WeaponHaloUpdate(ObjID objID, ObjID weapon, int numSubModels);

EXTERN void WeaponHaloInflate(ObjID objID, ObjID weapon);
EXTERN void WeaponHaloDeflate(ObjID objID, ObjID weapon);

EXTERN void WeaponHaloSetBlock(ObjID objID, ObjID weapon, int submod);
EXTERN void WeaponHaloUnsetBlock(ObjID objID, ObjID weapon);

////////////////////////////////////////

#else

#define kWH_NumModels 0

#define InitWeaponHaloSystem()
#define TermWeaponHaloSystem()

#define WeaponHaloInit(a,b,c)
#define WeaponHaloTerm(a,b)

#define WeaponHaloUpdate(a,b,c)

#define WeaponHaloInflate(a,b)
#define WeaponHaloDeflate(a,b)

#endif

