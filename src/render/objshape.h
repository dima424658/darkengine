/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objshape.h,v 1.7 2000/01/31 09:52:56 adurant Exp $
// this module provides basic info about an object, based on its renderer model
#pragma once

#ifndef __OBJSHAPE_H
#define __OBJSHAPE_H

#include <objtype.h>
#include <matrixs.h>

// gets the radius or the bbox for a model
// radius returns 0 on fail, GetBBox returns null, doesnt change bmin or bmax

// gets bbox in scaled obj space - hence rotations, and translations have no effect
// on bbox.
EXTERN BOOL ObjGetObjRelBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax);

// get bbox in world space, but offset by object position.  hence translations have no effect
// on bbox, but scaling and rotations do,
EXTERN BOOL ObjGetObjOffsetBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax);

// get bbox in world space.
EXTERN BOOL ObjGetWorldBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax);

// Get vhot position in [scaled] obj spaced. But rotation has no effect
EXTERN void ObjGetObjRelVhot(ObjID obj,int vhot,mxs_vector* pos);

// get vhot pos in world space, but offset by object position.  hence translations have no effect
// on bbox, but scaling and rotations do,
// Currently only works for mds models
EXTERN void ObjGetObjOffsetVhot(ObjID obj,int vhot,mxs_vector* pos);


// get bbox in whichever coord system takes up least space and is easy to compute.
#define OBJ_BBOX_TYPE_NONE   -1
#define OBJ_BBOX_TYPE_OBJREL 0
#define OBJ_BBOX_TYPE_OBJOFF 1
#define OBJ_BBOX_TYPE_WORLD  2
EXTERN int ObjGetFavoriteBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax);

// get radius of object, including scaling.
EXTERN float ObjGetRadius(ObjID obj);

// gets the dimensions of the object relative bbox.
EXTERN BOOL ObjGetUnscaledDims(ObjID obj, mxs_vector* dims);

// trans is rotation matrix from bbox coord space to desired bounding coordinate space
EXTERN void BoundBBox(mxs_matrix *trans,mxs_vector *bmin,mxs_vector *bmax);

#endif



