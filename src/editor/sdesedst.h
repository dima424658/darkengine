/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/sdesedst.h,v 1.7 2000/01/29 13:13:11 adurant Exp $
#pragma once

#ifndef __SDESEDST_H
#define __SDESEDST_H

#include <rect.h>
#include <sdestype.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//
// Struct Editor Flags (part of the editor descriptor)
//

enum _eStructEditorFlags 
{
   kStructEditNoFieldNames  = (1 << 0),  // don't show field names
   kStructEditShowTypes     = (1 << 1),  // DO show types (defaults OFF!)
   kStructEditFriendlyTypes = (1 << 2),  // user-friendly type names (String instead of char*)
   kStructEditDoneButton    = (1 << 3),  // Add a "Done" button
   kStructEditCancelButton  = (1 << 4),  // Add a "Cancel" button
   kStructEditApplyButton   = (1 << 5),  // Add an "Apply" button
};

#define kStructEditAllButtons (kStructEditDoneButton | kStructEditCancelButton | kStructEditApplyButton)

// 
// Struct Editor Descriptor
//

struct StructEditorDesc 
{
   char title[32]; // Title string appearing at top of gadget;
   eStructEditorFlags flags;
};

////////////////////

//
// Edit event types
//

enum _eStructEditEventType
{
   kStrEdEventApply,    // Apply button was hit
   kStrEdEventFieldChange,   // A field was changed
   kStrEdEventDone,        // We're done
   kStrEdEventCancel,      // We've been cancelled.
};

//
// The event structure
//

struct StructEditEvent
{
   eStructEditEventType type; // what happened?
   StructEditor* editor;      // to whom?
   void* eventdata;           // event-type-specific info
};

//
// Struct Editor
//

#define MAX_FIELDS 32
#define MAX_STRLEN 32

enum eStructEditorStatusFlags
{
   kSdescInProgress = 1 << 0,
   kSdescOK         = 1 << 1,
};


struct StructEditor
{
   StructEditorDesc     editdesc;
   void*                cur_struc;
   const sStructDesc*   sdesc;
   int                  numbutts;
   StructEditCB         cb;
   StructEditCBData     cbdata;
   Rect*                rects;
   ulong*               p_flags;    // pointer to flag word
   ulong                def_flags;  // flag word if not modal

   HWND                 hWnd;
   HINSTANCE            hInst;
   HWND                 textbox[MAX_FIELDS];
   HFONT                hWndFont;
};

#endif // __SDESEDST_H



