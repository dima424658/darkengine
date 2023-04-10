/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/isdescty.h,v 1.4 1997/10/20 18:22:40 mahk Exp $
#pragma once  
#ifndef __ISDESCTY_H
#define __ISDESCTY_H

#include <sdestype.h>
#include <comtools.h>

F_DECLARE_INTERFACE(IStructEditor);

typedef struct sStructEditorDesc sStructEditorDesc;
typedef struct sStructEditEvent sStructEditEvent;
typedef void* StructEditCBData;

typedef void (LGAPI *StructEditCB)(sStructEditEvent* event, StructEditCBData data);

#endif // __ISDESCTY_H

