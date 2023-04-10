/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uiloop.h,v 1.4 2000/01/31 10:05:03 adurant Exp $
#pragma once

#ifndef __UILOOP_H
#define __UILOOP_H

//
// Loopmode factory
//

DEFINE_LG_GUID(LOOPID_UI, 0x25);
EXTERN struct sLoopClientDesc uiLoopClientDesc;

//
// Context for loopmode go 
// 

typedef struct _uiContext
{
   const char* cursor; // cursor bmap id
} uiLoopContext;

#endif // __UILOOP_H
