/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef EDITPLST_H
#define EDITPLST_H

#include <objtype.h>

typedef struct PropertyListEditorDesc
{
   char title[32];
   ulong flags;
} PropertyListEditorDesc;

typedef struct PropertyListEditor PropertyListEditor;

EXTERN PropertyListEditor *PropertyListEditorCreate(ObjID editID, PropertyListEditorDesc *editdesc);

#endif
