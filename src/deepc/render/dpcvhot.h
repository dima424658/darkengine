/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCVHOT_H
#define __DPCVHOT_H

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // !__MATRIXS_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

// Get the location of a vhot, taking into account joint rotations
EXTERN void VHotGetRaw(mxs_vector *pLoc, ObjID objID, int vhotNum);
// Get the location of a vhot, taking into account joint rotations
EXTERN void VHotGetLoc(mxs_vector *pLoc, ObjID objID, int vhotNum);
// Get the raw vhot offset
EXTERN void VHotGetOffset(mxs_vector *pOffset, ObjID objID, int vhotNum);

#endif // __DPCVHOT_H