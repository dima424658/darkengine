/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mnumprop.h,v 1.4 2000/01/31 09:52:53 adurant Exp $
#pragma once

//
// Model number property header
//

#ifndef __MNUMPROP_H
#define __MNUMPROP_H

#include <objtype.h>

#define PROP_MODELNUMBER_NAME "ModelNumber"

// init and term
EXTERN void ModelNumberPropInit(void);
EXTERN void ModelNumberPropTerm(void);

// access
EXTERN BOOL ObjGetModelNumber(ObjID obj, int *num);
EXTERN void ObjSetModelNumber(ObjID obj, int *num);

// Given an object, make sure its model is loaded
EXTERN void ObjLoadModel(ObjID obj);

// for convenience until tempobj dies
EXTERN int ObjGetModelType(ObjID obj);

#endif   // __MNUMPROP_H
