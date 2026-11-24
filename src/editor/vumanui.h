/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vumanui.h,v 1.7 2000/01/29 13:13:31 adurant Exp $
#pragma once

#include <region.h>
#include <event.h>

#ifndef __VUMANUI_H
#define __VUMANUI_H

//////////////////////////////////////////////////////////////
//   VIEW MANAGER GUI
//////////////////////////////////////////////////////////////

//
// Create the gui, given a bounding rect
//

EXTERN void vmCreateGUI(Region* parent, Rect* bounds);
EXTERN void vmDestroyGUI(void);

//
// Get a view's region
//

EXTERN Region vmRegions[];
#define vmGetRegion(x)  (&vmRegions[x])

//
// Reserve camera panes for an editor tool.
// 

typedef enum 
{
   vmGrabSingle,  // grab a single view
   vmGrabTwoWide, // grab two side-by-side views
   vmGrabTwoHigh, // grab two up-and-down views
   vmNumParms,
} vmGrabParm;


// Returns the reserved view mask, or zero if the panes are unavailable.
EXTERN int vmGrabViews(vmGrabParm parm, Rect *bounds);
EXTERN void vmGetReservedBounds(int mask, Rect *bounds);

// Return reserved panes to the camera layout.
EXTERN void vmReleaseViews(int mask);

// see if the views are all free
EXTERN BOOL vmAllViewsAvailable(void);

///////////////
// menuing horror control state

// is this a fuckup, or what - global state for menus in vm
// this means a menu is in progress, if you need to know
EXTERN BOOL vm_menu_inprog;
// set this if you want to leave "cur camera" focused
EXTERN BOOL vm_menu_camera_lock;
// will have mouse coordinates
EXTERN int vm_menu_click_x, vm_menu_click_y;

#endif // __VUMANUI_H

