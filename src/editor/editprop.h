/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editprop.h,v 1.5 2000/01/29 13:11:49 adurant Exp $
#pragma once

#ifndef __EDITPROP_H
#define __EDITPROP_H

#include <comtools.h>
#include <objtype.h>

//
// Object property editor UI
//

F_DECLARE_INTERFACE(IProperty);

typedef struct PropertyEditor PropertyEditor;
typedef struct PropEditEvent PropEditEvent;

typedef ulong ePropEditEventType;

//
// Callback stuff - one callback per PropertyEditor, please
//

typedef struct PropEditEvent PropEditEvent;
typedef void* PropEditCBData;
typedef void (*PropEditCB)(PropEditEvent* event, PropEditCBData data);

// Create a property editor for a property
EXTERN PropertyEditor* PropertyEditorCreate(IProperty* prop, ObjID objid);

// Install a callback for a property editor
EXTERN void PropertyEditorInstallCallback(PropertyEditor* ped, PropEditCB cb, PropEditCBData data);

// Destroy a property editor
EXTERN void PropertyEditorDestroy(PropertyEditor *ped);


#endif   // __EDITPROP_H
