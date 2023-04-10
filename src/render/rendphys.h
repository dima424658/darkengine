/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendphys.h,v 1.6 2000/01/31 09:53:11 adurant Exp $
#pragma once

#ifndef __RENDPHYS_H
#define __RENDPHYS_H

#include <objtype.h>
#include <matrixs.h>

EXTERN BOOL RendPhysGetSphereModelInfo(ObjID obj, mxs_vector *pos, mxs_real *size, int *num);
EXTERN BOOL RendPhysGetOBBModelInfo(ObjID obj, mxs_vector *pos, mxs_angvec *fac, mxs_vector *bbox);

EXTERN BOOL RendPhysGetVectors(ObjID obj, mxs_vector *pos, mxs_vector *velocity, mxs_vector *forces, mxs_vector constraints[], int *num_constraints);

EXTERN BOOL RendPhysGetPhysBBox(ObjID obj, mxs_vector *pos, mxs_angvec *fac, mxs_vector *bbox);

EXTERN BOOL RendCreatureGetJointInfo(ObjID obj, mxs_vector *pos, int *num);

#endif


