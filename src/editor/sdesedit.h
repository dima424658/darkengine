/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/sdesedit.h,v 1.6 2000/01/29 13:13:09 adurant Exp $
#pragma once

#ifndef __SDESEDIT_H
#define __SDESEDIT_H

#include <sdestype.h>

typedef struct StructEditor StructEditor;
typedef struct StructEditorDesc StructEditorDesc;

typedef ulong eStructEditorFlags;
typedef ulong eStructEditEventType;

//
// Attach a callback to a struct editor.  Each struct editor can have only
// one callback
//

typedef struct StructEditEvent StructEditEvent;
typedef void* StructEditCBData;

typedef void (*StructEditCB)(StructEditEvent* event, StructEditCBData data);

EXTERN void StructEditorInstallCallback(StructEditor* ed, StructEditCB cb, StructEditCBData data);

//
// Create a structure editor for a data block
// IF root is NULL, creates it in the LGadCurrentRoot();
// 

EXTERN StructEditor* StructEditorCreate(StructEditorDesc* editdesc, sStructDesc* sdesc, void* editme);

// if you want to have a struct editor BLOCK on its done/cancel 
// returns true if done, false if cancel
EXTERN BOOL StructEditorGoModal(StructEditor* ed);

//
// Accessors
//

EXTERN StructEditorDesc*   StructEditorGetDesc(StructEditor* ed);
EXTERN sStructDesc*        StructEditorGetStructDesc(StructEditor* ed);
EXTERN void*               StructEditorGetStruct(StructEditor* ed);

//
// Destroy a struct editor
//

EXTERN void StructEditorDestroy(StructEditor* ed);

#endif // __SDESEDIT_H
