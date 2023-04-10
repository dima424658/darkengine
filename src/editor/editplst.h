/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef EDITPLST_H
#define EDITPLST_H

#include <objtype.h>
#include <editprop.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct PropertyListEditorDesc
{
   char title[32];
   ulong flags;      // no flags yet
};

#define EDPLIST_NUM_BUTTONS 3


struct PropertyListEditor
{
   ObjID             editID;
   int               mode;
   HWND              hWnd;
   HINSTANCE         hInst;
   HWND              hMainWnd;
   HWND              hWndPropBox;
   HWND              hWndMetaBox;
   HWND              hWndButtons[EDPLIST_NUM_BUTTONS];
   HFONT             hWndFont;
};

typedef struct PropertyListEditorDesc PropertyListEditorDesc;
typedef struct PropertyListEditor PropertyListEditor;

// Show list of properties for selected object.
EXTERN PropertyListEditor* PropertyListEditorCreate(ObjID editID, PropertyListEditorDesc* editdesc);

#endif









