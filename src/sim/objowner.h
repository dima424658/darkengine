/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objowner.h,v 1.2 2000/01/31 10:00:13 adurant Exp $
#pragma once

#ifndef OBJOWNER_H
#define OBJOWNER_H
#include <objtype.h>

//------------------------------------------------------------
// OBJECT OWNER FN PROTOS
//

EXTERN BOOL ObjGetOwner(ObjID obj, int *owner);
EXTERN BOOL ObjSetOwner(ObjID obj, int owner);

#endif // OBJOWNER_H
