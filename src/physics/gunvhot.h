/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __GUNVHOT_H
#define __GUNVHOT_H

#include <matrixs.h>
#include <objtype.h>

// Get the location of a vhot, taking into account joint rotations
EXTERN void VHotGetRaw(mxs_vector *pLoc, ObjID objID, int vhotNum);
// Get the location of a vhot, taking into account joint rotations
EXTERN void VHotGetLoc(mxs_vector *pLoc, ObjID objID, int vhotNum);
// Get the raw vhot offset
EXTERN void VHotGetOffset(mxs_vector *pOffset, ObjID objID, int vhotNum);

#endif // __GUNVHOT_H
