/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmterr.h,v 1.5 2000/01/31 09:51:59 adurant Exp $
//
// Physics moving terrain header
//
#pragma once

#ifndef __PHMTERR_H
#define __PHMTERR_H

#include <objtype.h>

EXTERN void ResetMovingTerrain();
EXTERN void RefreshMovingTerrain();

EXTERN void PostLoadMovingTerrain();

EXTERN void UpdateMovingTerrainVelocity(ObjID objID, ObjID next_node, float speed);

EXTERN void ActivateMovingTerrain(ObjID objID);
EXTERN void DeactivateMovingTerrain(ObjID objID);

EXTERN void UpdateMovingTerrain(int ms);

EXTERN BOOL IsMovingTerrain(ObjID objID);

#endif // __PHMTERR_H
