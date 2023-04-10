/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinvui.h,v 1.7 1998/12/15 11:14:23 mahk Exp $
#pragma once  
#ifndef __DRKINVUI_H
#define __DRKINVUI_H
#include <objtype.h>

////////////////////////////////////////////////////////////
// DARK INVENTORY UI API

EXTERN void InvUIInit(void);
EXTERN void InvUITerm(void);

// Game mode enter/exit
EXTERN void InvUIEnterMode(BOOL resuming);
EXTERN void InvUIExitMode(BOOL suspending);

// Force the display to refresh
EXTERN void InvUIRefresh(void);

// force a display refresh of a particular object 
EXTERN void InvUIRefreshObj(ObjID obj);

// render the display, takes the absolute time, in msec 
EXTERN void InvUIRender(ulong msec);

// abort all frobs that are going on
EXTERN void InvUIAbortAllFrobs(void);

// Is it ok to give this particular object world focus
EXTERN BOOL ValidWorldFocusTarget(ObjID obj); 

// Draw string name and count for object, at (0,0) (wid,hgt)
EXTERN void AnnotateObject(ObjID obj, int wid, int hgt); 

#endif // __DRKINVUI_H
