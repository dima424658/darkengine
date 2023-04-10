/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objhp.h,v 1.3 2000/01/29 13:41:19 adurant Exp $
#pragma once

#ifndef OBJHP_H
#define OBJHP_H
#include <objtype.h>

//------------------------------------------------------------
// OBJECT HIT POINTS FN PROTOS
//

EXTERN BOOL ObjGetHitPoints(ObjID obj, int *hp);
EXTERN BOOL ObjSetHitPoints(ObjID obj, int hp);

EXTERN BOOL ObjGetMaxHitPoints(ObjID obj, int *hp);
EXTERN BOOL ObjSetMaxHitPoints(ObjID obj, int hp);

#endif // OBJHP_H
