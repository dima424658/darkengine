/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objslit.h,v 1.4 2000/01/31 09:52:57 adurant Exp $
#pragma once

#ifndef OBJSLIT_H
#define OBJSLIT_H
#include <objtype.h>


// OBJECT SELF-ILLUMINATION LEVEL FN PROTOS
EXTERN BOOL ObjGetSelfLit(ObjID obj, int *psl);
EXTERN BOOL ObjSetSelfLit(ObjID obj, int sl);

// Update all self-lit objects
EXTERN void SelfLitUpdateAll(void); 


// OBJECT SHADOW FN PROTOS
EXTERN BOOL ObjGetShadow(ObjID obj, int *psl);
EXTERN BOOL ObjSetShadow(ObjID obj, int sl);

// Update all shadows
EXTERN void ShadowUpdateAll(void); 

#endif // OBJSLIT_H
