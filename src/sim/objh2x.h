/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objh2x.h,v 1.3 2000/01/29 13:41:18 adurant Exp $
#pragma once

#ifndef OBJH2X_H
#define OBJH2X_H
#include <objtype.h>

//------------------------------------------------------------
// OBJECT H2X LEVEL FN PROTOS
//

EXTERN BOOL ObjGetH2X(ObjID obj, int *ph2x);
EXTERN BOOL ObjSetH2X(ObjID obj, int h2x);

EXTERN BOOL ObjGetMaxH2X(ObjID obj, int *ph2x);
EXTERN BOOL ObjSetMaxH2X(ObjID obj, int ph2x);

EXTERN BOOL ObjAdjustH2X(ObjID obj, int h2xOffset);

#endif // OBJH2X_H
