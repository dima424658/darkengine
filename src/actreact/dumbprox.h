/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/dumbprox.h,v 1.1 1998/06/19 00:11:40 dc Exp $

#pragma once

#ifndef __DUMBPROX_H
#define __DUMBPROX_H

#include <propface.h>

typedef cDynArray<ObjID> cDynObjArray;

// return whether or not to abort the prox check
typedef BOOL (*ProxCallback)(ObjID obj, float rad_2);

// build a prox prop named prox_name
EXTERN IBoolProperty *ProxBuildProp(const char *prox_name);

// check within rad for prox's conforming to pProx
// if objList, fill it in, if pCB, call it with each, if both, do both

// pass in a position
EXTERN BOOL ProxCheckLoc(IBoolProperty *pProx, mxs_vector *src_pos, float rad, cDynObjArray *objList, ProxCallback pCB);
// pass in an object
EXTERN BOOL ProxCheckObj(IBoolProperty *pProx, ObjID src, float rad, cDynObjArray *objList, ProxCallback pCB);

#endif  // __DUMBPROX_H
