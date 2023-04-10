/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editinh.h,v 1.3 2000/01/29 13:11:37 adurant Exp $
// Inheritance editor API
#pragma once

#ifndef __EDITINH_H
#define __EDITINH_H

#include <edinhtyp.h>
#include <objtype.h>

EXTERN InheritanceEditor* InheritanceEditorFancyCreate(ObjID id, ObjID root, BOOL concrete, BOOL to_obj, InheritanceEditorDesc* editdesc);
EXTERN InheritanceEditor* InheritanceEditorCreate(ObjID id, InheritanceEditorDesc* editdesc);
EXTERN void InheritanceEditorDestroy(InheritanceEditor* ed);

#endif // __EDITINH_H
