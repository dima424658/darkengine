/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tfint.h,v 1.2 2000/01/31 09:45:40 adurant Exp $
#pragma once

#ifndef TFINT_H
#define TFINT_H
#include <objtype.h>

//------------------------------------------------------------
// TrapFlags FN PROTOS
//


#define TRAPF_NONE   0x000
#define TRAPF_ONCE   0x001
#define TRAPF_INVERT 0x002
#define TRAPF_NOON   0x004
#define TRAPF_NOOFF  0x008

#define PROP_TF_NAME "TrapFlags"

EXTERN BOOL ObjGetTrapFlags(ObjID obj, int *tf);
EXTERN BOOL ObjSetTrapFlags(ObjID obj, int tf);

#endif // TFINT_H
