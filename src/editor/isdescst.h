/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/isdescst.h,v 1.3 1997/10/21 21:00:21 mahk Exp $
#pragma once  
#ifndef __ISDESCST_H
#define __ISDESCST_H

//
// Struct Editor Events
//

enum eStructEditEvent_
{
   kStructEdOK    = 1 << 0,
   kStructEdApply = 1 << 1,
   kStructEdCancel  = 1 << 2,
};

struct sStructEditEvent
{
   ulong kind;
   IStructEditor* ed;
};

//
// Struct Editor Descriptor
//

enum _eStructEditorFlags 
{
   kStructEditNoFieldNames  = (1 << 0),  // don't show field names
   kStructEditShowTypes     = (1 << 1),  // DO show types (defaults OFF!)  (NIY)
   kStructEditFriendlyTypes = (1 << 2),  // user-friendly type names (String instead of char*) (NIY)
   kStructEditNoOKButton    = (1 << 3),  // Remove the "OK" button 
   kStructEditNoCancelButton  = (1 << 4),  // Remove the "Cancel" button
   kStructEditNoApplyButton   = (1 << 5),  // Remove the "Apply" button
   kStructEditAllButtons      = 0, 
   kStructEditNoButtons       = (kStructEditNoOKButton|kStructEditNoCancelButton|kStructEditNoApplyButton), 
};

struct sStructEditorDesc
{
   char title[32]; // title string
   ulong flags;    // see above
};

#endif // __ISDESCST_H









