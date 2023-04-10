/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editprst.h,v 1.5 2000/01/29 13:11:50 adurant Exp $
#pragma once

#ifndef __EDITPRST_H
#define __EDITPRST_H

#include <objtype.h>
#include <propstor.h>
#include <isdescty.h>

//
// Callback events
//

enum _ePropEditEventType
{
   kPropEdEventDone,        // We're done
   kPropEdEventCancel,      // We've been cancelled.
};

struct PropEditEvent
{
   ePropEditEventType type;   // what happened?
   PropertyEditor* editor;    // to whom?
   void* eventdata;           // event-type-specific info
};

//
// The actual property editor
//

struct PropertyEditor
{
   void*          struc;
   ulong          size;
   ObjID          objid;
   IPropertyStore*  propraw;
   IStructEditor*  sed;
   PropEditCB     cb;
   PropEditCBData cbdata;
};

#endif   // __EDITPRST_H


