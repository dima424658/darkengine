/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objscale.h,v 1.4 2000/01/31 09:52:55 adurant Exp $
// note: this is really in ScalProp.cpp, not objscale.cpp, for some reason ??
#pragma once

#ifndef OBJSCALE_H
#define OBJSCALE_H
#include <objtype.h>
#include <matrixs.h>

//------------------------------------------------------------
// OBJECT SCALE PROTOTYPES
//

EXTERN BOOL ObjGetScale(ObjID obj, mxs_vector *scale);
EXTERN BOOL ObjSetScale(ObjID obj, mxs_vector *scale);
EXTERN void ObjResetScale(ObjID obj);

#endif // OBJSCALE_H
