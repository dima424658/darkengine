/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objmagic.h,v 1.2 2000/01/29 13:41:22 adurant Exp $
#pragma once

#ifndef OBJMAGIC_H
#define OBJMAGIC_H
#include <objtype.h>
#include <propface.h>

//------------------------------------------------------------
// OBJECT AIR MAGIC FN PROTOS
//

EXTERN BOOL ObjGetMagic(ObjID obj, Label *magic);
EXTERN BOOL ObjSetMagic(ObjID obj, Label *magic);
EXTERN void ObjCreateMagic(ObjID obj);

#endif // OBJMAGIC_H
